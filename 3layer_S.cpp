#include "AIController.h"
#include <utility>
#include<queue>
#include<fstream>
#include<algorithm>
#include<random>
extern int ai_side;
std::string ai_name = "TEST_Onsearch";
using pri=std::pair<int,int>;
using prcmd=std::pair<int, std::pair<int, int> >;
const int INF=3000000;
int rdcnt=0;
//#define BFSTEST
//#define TEST
//init function is called once at the beginning	s
const double scale=10;
class state
{
	public:
	int board[9][9];
	int Plankcnt,oppcnt;
	double alpha,beta;
	double evalue;
	int dis,oppdis;
	static const int boardsiz=9;
	static const int target=8;
	static const int avlmove=12;

	enum boardstate{
		Empty,Agent,Opp
	};
	int Place[10][10][2];
	int dy[15]={-1,1,0,0,-1,-1,1,1,-1,-1,1,1},dx[15]={0,0,-1,1,1,-1,-1,1,1,-1,-1,1};
	enum Move{
		Left,Right,Down,Up,Lup,Ldown,Rdown,Rup,UpL,DownL,DownR,UpR
	};
	bool avl[15]={1,1,1,1};//Movement available
	int Op[2],Ap[2];//Opponent-position Agent-position
	void init() {
		#ifdef TEST
		std::fstream f("board.data",std::ios::trunc|std::ios::out);
		std::fstream f1("BFS.data",std::ios::out|std::ios::trunc);
		f1.close();
		std::fstream f2("DIS.data",std::ios::out|std::ios::trunc);
		f2.close();
		#endif
		for(int i=0;i<boardsiz;++i)
			for(int j=0;j<boardsiz;++j)
			{	
				board[i][j]=Empty;
				Place[i][j][0]=Place[i][j][1]=false;
			}
		#ifdef TEST
		f<<"ai side:"<<ai_side<<"\n";
		f.close();
		
		#endif
		evalue=0;alpha=-INF;beta=INF;
		dis=INF;oppdis=INF;
		if(!ai_side)
		{
			Op[0]=0;Op[1]=4;
			Ap[0]=8;Ap[1]=4;
		}
		else
		{
			Op[0]=8;Op[1]=4;
			Ap[0]=0;Ap[1]=4;
		}
		board[Op[0]][Op[1]]=Opp;
		board[Ap[0]][Ap[1]]=Agent;
		Plankcnt=oppcnt=10;
	}
	state(){
	}
	state(const state &ot):Plankcnt(ot.Plankcnt),oppcnt(ot.oppcnt),alpha(ot.alpha),beta(ot.beta),evalue(ot.evalue)
	{
		dis=ot.dis;oppdis=ot.oppdis;
		for(int i=0;i<boardsiz;++i)
			for(int j=0;j<boardsiz;++j)
			{
				board[i][j]=ot.board[i][j];
				Place[i][j][0]=ot.Place[i][j][0];
				Place[i][j][1]=ot.Place[i][j][1];
			}
		Op[1]=ot.Op[1];Op[0]=ot.Op[0];
		Ap[1]=ot.Ap[1];Ap[0]=ot.Ap[0];
		for(int i=0;i<avlmove;++i)
			avl[i]=ot.avl[i];
	}
	inline void reset()
	{
		for(int i=4;i<avlmove;++i)
			avl[i]=0;
	}
	inline bool is_wall(int i,int x,int y)
	{
		int tx=x+dx[i],ty=y+dy[i];
		if(tx<0||tx>8||ty<0||ty>8||x<0||x>8||y<0||y>8)
			return true;
		switch (i)
		{
			case Left:return Place[x][y][1];
			case Right:return Place[tx][ty][1];
			case Down:return Place[tx][ty][0];
			case Up:return Place[x][y][0];
			case Lup:return Place[x][ty][0];
			case Ldown:return Place[tx][ty][0];
			case Rdown:return Place[tx][ty][0];
			case Rup:return Place[x][ty][0];
			case UpL:return Place[tx][y][1];
			case DownL:return Place[tx][y][1];
			case DownR:return Place[tx][ty][1];
			case UpR:return Place[tx][ty][1];
			default:
				std::cerr<<"[is_wall]Unexpected Move\n";
				return true;
		}
	}
	inline bool check_place(int x,int y,bool op)
	{
		if(x==boardsiz-1||y==boardsiz-1)
				return false;
		if(op)
		{
			if((Place[x][y+1][1]|Place[x+1][y+1][1])||(Place[x][y][0]&&Place[x][y+1][0]==Place[x][y+1][0]))
				return false;
		}
		else
		{
			if(Place[x][y][0]|Place[x][y+1][0]||(Place[x+1][y+1][1]&&Place[x][y+1][1]==Place[x+1][y+1][1]))
				return false;
		}
		return true;
	}
	inline std::pair<int,int> move(int i,int x,int y)//0-11
	{
		int tx=x+dx[i],ty=y+dy[i];
		if(is_wall(i,x,y))
			return std::pair<int,int>(x,y);
		if(board[tx][ty]==Opp)
		{
			//std::cerr<<"Countering Opponent! cors:"<<x<<" "<<y<<" "<<tx<<" "<<ty<<"\n";
			if(is_wall(i,tx,ty))
			{
				//std::cerr<<"Counter and wall"<<"\n";
				switch (i)
				{
					case Left:avl[Lup]=avl[Ldown]=1;break;
					case Right:avl[Rup]=avl[Rdown]=1;break;
					case Down:avl[DownL]=avl[DownR]=1;break;
					case Up:avl[UpL]=avl[UpR]=1;break;
				}
				return std::pair<int,int>(x,y);
			}
			else
			{	
				//std::cerr<<"Counter and Jump"<<"\n";
				return std::pair<int,int>(tx+dx[i],ty+dy[i]);
			}
		}
		else
			return std::pair<int,int>(tx,ty);
	}
	inline int bfs(int x0,int y0,int target_line)//return the minimum distance from AP/OP to border
	{
		if(x0==target_line)	{return 0;}
		std::queue<pri> q;
		
		//std::cerr<<"Begin:"<<q.size()<<"\n";
		int d[boardsiz][boardsiz];
		//std::cerr<<"d applied\n";
		for(int i=0;i<boardsiz;++i)
			for(int j=0;j<boardsiz;++j)
				d[i][j]=INF;
		d[x0][y0]=0;
		q.push(std::make_pair(x0,y0));
		while(!q.empty())
		{
			int x=q.front().first,y=q.front().second;
			//q.pop();
			for(int i=0;i<avlmove;++i)
				if(avl[i])
				{
					std::pair<int,int> target(move(i,x,y));
					if(target==q.front())
						continue;
					if(d[target.first][target.second]==INF)
					{
						d[target.first][target.second]=d[x][y]+1;
						q.push(target);
						/*test*/
						//if(rdcnt==3&&board[target.first][target.second]==0)board[target.first][target.second]=4+(x0==Ap[0]?1:0);
						
						if(target.first==target_line)
						{
							if(x0==Ap[0]&&y0==Ap[1])
								dis=d[target.first][target.second];
							if(x0==Op[0]&&y0==Op[1])
								oppdis=d[target.first][target.second];
							return d[target.first][target.second];
						}
					}	
				}
			reset();
			q.pop();
		}
		return INF;
	}
	inline void upd(std::pair<int, std::pair<int, int> > loc)
	{
		int x=loc.second.first,y=loc.second.second;
		switch (loc.first)
		{
			case -1:break;
			case 0:board[Op[0]][Op[1]]=Empty;Op[0]=x;Op[1]=y;board[Op[0]][Op[1]]=Opp;break;
			case 1:Place[x][y+1][1]=Place[x+1][y+1][1]=10+(oppcnt--);break;//CHECK
			case 2:Place[x][y][0]=Place[x][y+1][0]=10+(oppcnt--);break;
			case 3:board[Ap[0]][Ap[1]]=Empty;Ap[0]=x;Ap[1]=y;board[Ap[0]][Ap[1]]=Agent;break;
			case 4:Place[x][y+1][1]=Place[x+1][y+1][1]=Plankcnt--;break;//CHECK
			case 5:Place[x][y][0]=Place[x][y+1][0]=Plankcnt--;break;
			default:
				std::cerr<<"[Updating Board] Unknown Command\n";
				break;
		}
	}
	inline void undo_place(int x,int y,bool op,bool opp)//assert undoing is possible
	{
		if(op)
			Place[x][y+1][op]=Place[x+1][y+1][op]=0;
		else
			Place[x][y][0]=Place[x][y+1][0]=0;
		if(!opp)
			++Plankcnt;
		else
			++oppcnt;
	}
	inline prcmd random_move()
	{
		int a[8]={0,1,2,3,4,5,6,7};
		std::shuffle(a,a+4,std::default_random_engine(time(NULL)));
		if(!Plankcnt)
		{
			for(int i=0;i<8;i++)
				if(avl[a[i]])
				{
					pri tmp(move(a[i],Ap[0],Ap[1]));
					#ifdef TEST
					std::fstream f("BFS.data",std::ios::out|std::ios::app);
					f<<tmp.first<<" "<<tmp.second<<"\n";
					f.close();
					#endif
					if(tmp.first!=Ap[0]||tmp.second!=Ap[1])
						return std::make_pair(0,tmp);
				}
			reset();
		}
		for(int i=0;i<boardsiz-1;++i)
			for(int j=0;j<boardsiz-1;++j)
			{
				int tx=std::rand()%(boardsiz-1),ty=std::rand()%(boardsiz-1);
				if(check_place(tx,ty,tx%2?true:false))
					return std::make_pair(2-(tx%2),std::make_pair(tx,ty));
			}
		return std::make_pair(-1,std::make_pair(-1,-1));
	}
	inline double minus_relu(double x)
	{
		return 30-x;
	}
	inline double e_relu(double x)
	{
		return x;
	}
	inline double calc()//Evaluation Function
	{
		dis=bfs(Ap[0],Ap[1],ai_side?8:0);
		oppdis=bfs(Op[0],Op[1],ai_side?0:8);
		if(oppdis==INF||dis==INF)
		{	std::cerr<<"error dis\n";}
		return (20-e_relu(dis))*(20-e_relu(dis))+e_relu(oppdis)*e_relu(oppdis);
	}
	inline void print()
	{
		
		std::fstream f("board.data",std::ios::out|std::ios::app);
		if(!f.good())
		{	std::cerr<<"SHIT\n";return;}
		//bfs();
		f<<"[round "<<rdcnt<<"]:\n";
		f<<Ap[0]<<" "<<Ap[1]<<'\n'<<Op[0]<<' '<<Op[1]<<'\n';
 		f<<"  ";
		for(int i=0;i<boardsiz;++i)
			f<<' '<<i;
		f<<'\n';
		for(int i=0;i<boardsiz;++i)
		{
			f<<i<<' ';
			for(int j=0;j<boardsiz;++j)
			{
				char ch=Place[i][j][1]?'|':'=';
				f<<ch;
				ch=' ';
				if(board[i][j]==Agent)
					ch='A';
				if(board[i][j]==Opp)
					ch='O';
				if(board[i][j]==4)
					ch='E';
				if(board[i][j]==5)
					ch='U';
				f<<ch;
			}
			f<<"\n  ";
			for(int j=0;j<boardsiz;++j)
			{
				
				f<<" ";
				char ch=Place[i][j][0]?'-':'+';
				f<<ch;
			}
			f<<'\n';
		}
		f.close();
	}
};
state ST;
int d0;
int cnt=0;
prcmd step;
double Minimax_search(state &now,int d,bool op)
{
	prcmd ans;
	if(now.bfs(now.Ap[0],now.Ap[1],ai_side?8:0)==INF||now.bfs(now.Op[0],now.Op[1],ai_side?0:8)==INF)
	{	
		return -1;
	}
	++cnt;
	if(!d)
	{
		now.evalue=now.calc();
		//std::cerr<<"Value:"<<now.evalue<<" \n";
		return now.evalue;
	}
	state nxt(now);double tmp;
	if(op)
	{
		for(int i=0;i<now.avlmove;++i)
		{
			if(now.avl[i])
			{
				nxt.alpha=now.alpha;
				nxt.beta=now.beta;
				pri t=now.move(i,now.Ap[0],now.Ap[1]);
				if(t==pri(now.Ap[0],now.Ap[1]))continue;
				nxt.upd(prcmd(3,t));
				tmp=Minimax_search(nxt,d-1,!op);
				if(now.alpha<tmp&&tmp!=-1)
				{
					now.alpha=tmp;
					ans=prcmd(0,t);
				}
				if(now.alpha>now.beta)
					return -1;
				nxt.upd(prcmd(3,pri(now.Ap[0],now.Ap[1])));//Reset
			}
		}
		now.reset();
		if(now.Plankcnt)
		{
			for(int i=0;i<now.boardsiz-1;++i)
				for(int j=0;j<now.boardsiz-1;++j)
				{
					for(int k=0;k<2;++k)
						if(now.check_place(i,j,k))
						{
							nxt.alpha=now.alpha;
							nxt.beta=now.beta;
							nxt.upd(prcmd(5-k,pri(i,j)));
							tmp=Minimax_search(nxt,d-1,!op);
							if(now.alpha<tmp&&tmp!=-1)
							{
								now.alpha=tmp;
								ans=prcmd(2-k,pri(i,j));
							}
							if(now.alpha>now.beta)
								return -1;
							nxt.undo_place(i,j,k,0);
						}
				}
		}
	}
	else
	{
		double minvalue=INF;
		for(int i=0;i<now.avlmove;++i)
		{
			if(now.avl[i])
			{
				nxt.alpha=now.alpha;
				nxt.beta=now.beta;
				pri t=now.move(i,now.Op[0],now.Op[1]);
				if(t==pri(now.Op[0],now.Op[1]))continue;
				nxt.upd(prcmd(0,t));
				tmp=Minimax_search(nxt,d-1,!op);
				if(now.beta>tmp&&tmp!=-1)
					now.beta=tmp;
				if(now.alpha>now.beta)
					return -1;
				nxt.upd(prcmd(0,pri(now.Op[0],now.Op[1])));//reset
			}
		}
		now.reset();
		if(now.oppcnt)
		{
			for(int i=0;i<now.boardsiz-1;++i)
				for(int j=0;j<now.boardsiz-1;++j)
				{
					for(int k=0;k<2;++k)
						if(now.check_place(i,j,k))
						{
							nxt.alpha=now.alpha;
							nxt.beta=now.beta;
							nxt.upd(prcmd(2-k,pri(i,j)));
							tmp=Minimax_search(nxt,d-1,!op);
							minvalue=std::min(minvalue,tmp);
							if(now.alpha>now.beta)
								return -1;
							nxt.undo_place(k,i,j,1);
						}
				}
		}
	}
	if(d==d0)
	{	std::cerr<<"LE\n";step=ans;}
	if(op)
		now.evalue=now.alpha;
	else
		now.evalue=now.beta;
	return now.evalue;
}
void init() {
	
	ST.init();
	//d0=ai_side?1:2;
	srand(time(NULL));
}
state tmpST;
std::pair<int, std::pair<int, int> > action(std::pair<int, std::pair<int, int> > loc) {
	/* Your code here */
	d0=3;
	cnt=0;
	++rdcnt;
	ST.upd(loc);
	tmpST=ST;
	double tmp=Minimax_search(tmpST,d0,1);
	std::cerr<<"[round "<<rdcnt<<"]"<<"\n";
	ST.upd(prcmd(step.first+3,step.second));
	std::cerr<<"alpha:"<<tmpST.alpha<<"\nbeta:"<<tmpST.beta<<"\n";
	std::cerr<<"value:"<<tmpST.evalue<<"\n";
	std::cerr<<"searchcnt:"<<cnt<<"\n";
	#ifdef TEST
	std::cerr<<"Besr rate:"<<tmp<<"\n";
	
	std::cerr<<step.first<<" "<<step.second.first<<" "<<step.second.second<<"\n";
	std::cerr<<ST.bfs(ST.Ap[0],ST.Ap[1],ai_side?8:0)<<" "<<ST.bfs(ST.Op[0],ST.Op[1],ai_side?0:8)<<"\n";
	
	ST.print();
	std::fstream f("DIS.data",std::ios::out|std::ios::app);
	f<<ST.bfs(ST.Op[0],ST.Op[1],ai_side?0:8)<<"\n";
	f.close();
	#endif
	return step;
}