#include "AIController.h"
#include <utility>
#include<queue>
#include<fstream>
#include<algorithm>
#include<random>
#include<cstring>
extern int ai_side;
std::string ai_name = "TEST_Onsearch";
using pri=std::pair<int,int>;
using prcmd=std::pair<int, std::pair<int, int> >;
using prd=std::pair<int,double>;
const int INF=3000000;
int rdcnt=0;
//#define BFSTEST
//#define TEST
//#define DEBUG
//init function is called once at the beginning
const double scale=10;
class cmp
{
	public:
	bool operator()(const prd &a,const prd &b)const
	{
		return a.second<b.second;
	}
};
class state
{
	public:
	bool board[17][17];
	int Plankcnt,oppcnt;
	double alpha,beta;
	double evalue;
	int dis,oppdis;
	int d[17][17];
	int var[9],opvar[9];
	bool side;
	static const int boardsiz=17;
	static const int target=8;
	static const int avlmove=16;
	static const int ordersiz=128;
	int dy[17]={-2,2,0,0,-2,-2,2,2,-2,-2,2,2,-4,4,0,0},dx[17]={0,0,-2,2,2,-2,-2,2,2,-2,-2,2,0,0,-4,4};
	enum Move{
		Left,Right,Down,Up,Lup,Ldown,Rdown,Rup,UpL,DownL,DownR,UpR,LeftL,RightR,DownD,UpU
	};
	int Op[2],Ap[2];//Opponent-position Agent-position
	std::priority_queue<prd,std::vector<prd>,cmp> hp;
	void init() {
		memset(board,0,sizeof(board));
		evalue=0;alpha=-INF;beta=INF;
		dis=INF;oppdis=INF;
		side=1;//my_side
		if(!ai_side)
		{
			Op[0]=0;Op[1]=8;
			Ap[0]=16;Ap[1]=8;
		}
		else
		{
			Op[0]=16;Op[1]=8;
			Ap[0]=0;Ap[1]=8;
		}
		board[Op[0]][Op[1]]=true;
		board[Ap[0]][Ap[1]]=true;
		Plankcnt=oppcnt=10;
	}
	state(){
	}
	state(const state &ot):Plankcnt(ot.Plankcnt),oppcnt(ot.oppcnt),alpha(ot.alpha),beta(ot.beta)
	{
		for(int i=0;i<boardsiz;++i)
			for(int j=0;j<boardsiz;++j)
				board[i][j]=ot.board[i][j];
		for(int i=0;i<9;++i)
		{
			var[i]=ot.var[i];
			opvar[i]=ot.opvar[i];
		}
		Op[0]=ot.Op[0];Op[1]=ot.Op[1];
		Ap[0]=ot.Ap[0];Ap[1]=ot.Ap[1];
		side=ot.side;
		dis=ot.dis;oppdis=ot.oppdis;
		evalue=ot.evalue;
	}
	inline prcmd decode(int x)
	{
		if(x>=144||x<0)
		{	
			std::cerr<<x<<'\n';
			throw("Decode Error");
		}
		if(x>=128)
		{	
			int tx=(side?Ap[0]:Op[0])+dx[x-128],ty=(side?Ap[1]:Op[1])+dy[x-128];
			return prcmd(side?3:0,pri(tx,ty));
		}
		if(x>=64)
			return prcmd(side?5:2,pri(((x-64)/8)*2,((x-64)%8)*2));
		return prcmd(side?4:1,pri((x/8)*2,(x%8)*2));

	}
	inline bool check_place(int x,int y,bool op)
	{
		if(x==boardsiz-1||y==boardsiz-1)
				return false;
		if(op)
			return !(board[x][y+1]|board[x+1][y+1]|board[x+2][y+1]);
		else
			return !(board[x+1][y]|board[x+1][y+1]|board[x+1][y+2]);
	}
	inline bool is_wall(int i,int x,int y)
	{
		int tx=x+dx[i],ty=y+dy[i];
		if(tx<0||tx>=boardsiz||ty<0||ty>=boardsiz)
			return true;
		switch (i)
		{
			case Left:return board[x][y-1];
			case Right:return board[x][y+1];
			case Down:return board[x-1][y];
			case Up:return board[x+1][y];
			case Lup:return board[tx-1][ty];
			case Ldown:return board[tx+1][ty];
			case Rdown:return board[tx+1][ty];
			case Rup:return board[tx-1][ty];
			case UpL:return board[tx][ty+1];
			case DownL:return board[tx][ty+1];
			case DownR:return board[tx][ty-1];
			case UpR:return board[tx][ty-1];
			case UpU:
			case DownD:
			case LeftL:
			case RightR:return false;
			default:
				std::cerr<<"[is_wall]Unexpected Move\n";
				return true;
		}
	}
	inline std::pair<int,int> try_move(int i,int x,int y,bool avl[],bool ignore,bool output=0)//0-15
	{
		int tx=x+dx[i],ty=y+dy[i];
		if(is_wall(i,x,y))
			return std::pair<int,int>(x,y);
		if(output)
			std::cerr<<x<<" "<<y<<" "<<tx<<" "<<ty<<"\n";
		if(board[tx][ty]&&(!ignore))
		{
			if(output)
				std::cerr<<"is wall\n";
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
			}
			else
			{	
				switch (i)
				{
					case Left:avl[LeftL]=1;break;
					case Right:avl[RightR]=1;break;
					case Down:avl[DownD]=1;break;
					case Up:avl[UpU]=1;break;
				}
			}
			return std::pair<int,int>(x,y);
		}
		else
			return std::pair<int,int>(tx,ty);
	}
	inline int bfs(int x0,int y0,int target_line,bool output=0)//return the minimum distance from (x0,y0) to target_line
	{
		bool avl[avlmove]={1,1,1,1};
		if(x0==target_line)	{return 0;}
		std::queue<pri> q;
		//std::cerr<<"Begin:"<<q.size()<<"\n";
		//std::cerr<<"d applied\n";
		for(int i=0;i<boardsiz;++i)
			for(int j=0;j<boardsiz;++j)
				d[i][j]=INF;
		d[x0][y0]=0;
		q.push(std::make_pair(x0,y0));
		state tmp(*this);
		while(!q.empty())
		{
			int x=q.front().first,y=q.front().second;
			//q.pop();
			for(int i=4;i<avlmove;++i)
				avl[i]=false;
			for(int i=0;i<avlmove;++i)
				if(avl[i])
				{
					std::pair<int,int> target(try_move(i,x,y,avl,true));
					if(target==q.front())
						continue;
					if(d[target.first][target.second]==INF)
					{
						d[target.first][target.second]=d[x][y]+1;
						if(target.first!=target_line)
							q.push(target);
						if(output)
							tmp.board[target.first][target.second]=true;
					}	
				}
			q.pop();
		}
		int ans=INF;
		for(int i=0;i<boardsiz;i+=2)
		{	
			if(x0==Op[0]&&y0==Op[1])
				var[i>>1]=d[target_line][i];
			if(x0==Ap[0]&&y0==Ap[1])
				opvar[i>>1]=d[target_line][i];
			ans=std::min(ans,d[target_line][i]);
		}
		if(output)
			tmp.print();
		return ans;
	}
	void upd_dis()
	{
		dis=bfs(Ap[0],Ap[1],ai_side?16:0);
		oppdis=bfs(Op[0],Op[1],ai_side?0:16);
	}
	inline void upd(std::pair<int, std::pair<int, int> > loc)//0-16
	{
		int x=loc.second.first,y=loc.second.second;
		switch (loc.first)
		{
			case -1:break;
			case 0:board[Op[0]][Op[1]]=false;Op[0]=x;Op[1]=y;board[Op[0]][Op[1]]=true;break;
			case 1:board[x][y+1]=board[x+1][y+1]=board[x+2][y+1]=true;--oppcnt;break;//CHECK
			case 2:board[x+1][y]=board[x+1][y+1]=board[x+1][y+2]=true;--oppcnt;break;
			case 3:board[Ap[0]][Ap[1]]=false;Ap[0]=x;Ap[1]=y;board[Ap[0]][Ap[1]]=true;break;
			case 4:board[x][y+1]=board[x+1][y+1]=board[x+2][y+1]=true;--Plankcnt;break;//CHECK
			case 5:board[x+1][y]=board[x+1][y+1]=board[x+1][y+2]=true;--Plankcnt;break;
			default:
				std::cerr<<"[Updating Board] Unknown Command\n";
				break;
		}
	}
	inline void undo_place(int x,int y,bool op)//assert undoing is possible
	{
		if(op)
			board[x][y+1]=board[x+1][y+1]=board[x+2][y+1]=false;
		else
			board[x+1][y]=board[x+1][y+1]=board[x+1][y+2]=false;
		if(side)
			++Plankcnt;
		else
			++oppcnt;
	}
	inline double minus_relu(double x)
	{
		return 30-x;
	}
	inline double e_relu(double x)
	{
		return x;
	}
	inline double sigmoid(double x,double offset=0)
	{
		return 1/(1+exp(-x+offset));
	}
	inline int calc_cnt(int d[])
	{
		int cnt=0;
		for(int i=0;i<9;++i)
			if(d[i]!=INF)
				++cnt;
		return cnt;
	}
	inline int calc_side_con()
	{
		int buf=0,target_line=side?16:0;
		for(int i=1;i<9;++i)
		{
			while((!board[target_line][(i<<1)|1]||var[i]==INF)&&i<9)
				++i;
			++buf;
		}
		return buf;
	}
	inline int calc_opside_con()
	{
		int buf=0,target_line=side?0:16;
		for(int i=1;i<9;++i)
		{
			while((!board[target_line][(i<<1)|1]||opvar[i]==INF)&&i<9)
				++i;
			++buf;
		}
		return buf;
	}
	inline double calc_var(int d[])
	{
		double var=0,mean=0;
		int cnt=calc_cnt(d);
		for(int i=0;i<9;++i)
			if(d[i]!=INF)
				mean+=d[i];
		mean/=cnt;
		for(int i=0;i<9;++i)
			if(d[i]!=INF)
				var+=(d[i]-mean)*(d[i]-mean);
		return var/cnt;
	}
	inline double calc()//Evaluation Function
	{
		if(oppdis==INF||dis==INF)
		{	std::cerr<<"error dis\n";}
		evalue=-dis*dis;
		double varm=calc_var(var),varop=calc_var(opvar);
		double oppscale=400,varscale=8;//Hyperperameter
		int cnt=calc_cnt(var),opcnt=calc_cnt(opvar);
		double penalty_plank=150*sigmoid(0.75*(Plankcnt));
		return 10000-300*sigmoid(2*dis,3)-70*dis+500*sigmoid(2*oppdis,1)+60*oppdis+200*sigmoid(5*cnt,5)+penalty_plank;
	}
	inline void check_move(bool output=0)
	{
		bool avl[avlmove]={1,1,1,1};
		while(!hp.empty())hp.pop();
		pri origin(side?pri(Ap[0],Ap[1]):pri(Op[0],Op[1]));
		#ifdef DEBUG
			std::cerr<<"[heap cleared]\n"<<"Opp:"<<Op[0]<<" "<<Op[1]<<"\n";
			std::cerr<<"Agent:"<<Ap[0]<<" "<<Ap[1]<<"\n";
		#endif
		for(int i=0;i<avlmove;++i)
			if(avl[i])
			{
				pri target(try_move(i,origin.first,origin.second,avl,false,output));
				if(target==origin)continue;
				upd(prcmd(side?3:0,target));
				upd_dis();
				upd(prcmd(side?3:0,origin));
				if(output)
				{
					std::cerr<<i<<" "<<dis<<" "<<oppdis<<"\n";
					for(int i=0;i<avlmove;++i)
						std::cerr<<(avl[i]?'Y':'N')<<" ";
					std::cerr<<"\n";
				}
				if(dis==INF||oppdis==INF)
					continue;
				#ifdef DEBUG
					std::cerr<<dis<<" "<<oppdis<<'\n';
					std::cerr<<"[pushing move]"<<"move:"<<i<<" rate:"<<calc()<<"\n"<<"target:"<<target.first<<" "<<target.second<<"\n";
					if(!side)print();
				#endif
				
				if(output)std::cerr<<"[pushing move]"<<"move:"<<i<<" rate:"<<calc()<<"\n"<<"target:"<<target.first<<" "<<target.second<<"\n";
				if(side)
					hp.push(prd(128+i,calc()));
				else
					hp.push(prd(128+i,-1*calc()));
				
			}
		if((side?Plankcnt:oppcnt)!=0)
		{
			//int sty=std::max(0,(side?Op[1]:Ap[1])-6),eny=std::min(boardsiz-3,(side?Op[1]:Ap[1])+6);
			//int stx=std::max(0,(side?Op[0]:Ap[0])-6),enx=std::min(boardsiz-3,(side?Op[1]:Ap[1])+6);
			for(int i=0;i<boardsiz;i+=2)
				for(int j=0;j<boardsiz;j+=2)
				{
					if(check_place(i,j,0))
					{
						upd(prcmd(side?5:2,pri(i,j)));
						upd_dis();
						undo_place(i,j,0);
						if(dis==INF||oppdis==INF)
							continue;
						if(side)
							hp.push(prd(64+(i>>1)*8+(j>>1),calc()));
						else
							hp.push(prd(64+(i>>1)*8+(j>>1),-1*calc()));
						//std::cerr<<calc()<<" ";
					}
					if(check_place(i,j,1))
					{
						upd(prcmd(side?4:1,pri(i,j)));
						upd_dis();
						undo_place(i,j,1);
						if(dis==INF||oppdis==INF)	
							continue;
						if(side)
							hp.push(prd((i>>1)*8+(j>>1),calc()));
						else
							hp.push(prd((i>>1)*8+(j>>1),-1*calc()));
						//std::cerr<<calc()<<" ";
					}
				}
		}
	}
	inline void print()
	{
		
		std::fstream f("board.data",std::ios::out|std::ios::app);
		if(!f.good())
		{	std::cerr<<"OPEN FILE ERROR\n";return;}
		//bfs();
		f<<"[round "<<rdcnt<<"]:\n";
		f<<Ap[0]/2<<" "<<Ap[1]/2<<'\n'<<Op[0]/2<<' '<<Op[1]/2<<'\n';
		f<<' ';
		for(int i=0;i<boardsiz;i+=2)
			f<<' '<<i/2;
		f<<'\n';
		for(int i=0;i<boardsiz;++i)
		{
			if(i&1)
				f<<"  ";
			else
				f<<i/2<<' ';
			for(int j=0;j<boardsiz;++j)
			{
				char ch=board[i][j]?'~':' ';
				if(i%2==0&&j%2==0&&board[i][j])
					ch='C';
				if(i==Ap[0]&&j==Ap[1])
					ch='A';
				if(i==Op[0]&&j==Op[1])
					ch='O';
				f<<ch;
			}
			f<<"\n";
		}
		f.close();
	}
};
state ST;
int d0;
int cnt=0;
prcmd step;
void init() {
	ST.init();
	srand(time(NULL));
	d0=3; 
	//d0=ai_side?1:2;
	#ifdef DEBUG
		std::fstream f("board.data",std::ios::out|std::ios::trunc);
		f.close();
	#endif
	srand(time(NULL));
}
double Minimax_Search(state cur,int d)
{
	++cnt;
	if(d!=d0)
		cur.side=!cur.side;
	cur.check_move();
	#ifdef DEBUG
		std::cerr<<"side:"<<(cur.side?'A':'O')<<"\n";
	#endif
	if(d==1)
	{	
		if(d==d0)
			step=cur.decode(cur.hp.top().first);
		return std::abs(cur.hp.top().second);
	}
	state nxt(cur);
	//nxt.print();
	prcmd cmd,endcmd;
	pri origin(cur.side?pri(cur.Ap[0],cur.Ap[1]):pri(cur.Op[0],cur.Op[1]));
	double tmp;
	if(cur.side)
	{
		while(!cur.hp.empty())
		{	
			cmd=cur.decode(cur.hp.top().first);
			nxt.upd(cmd);
			nxt.upd_dis();
			nxt.alpha=cur.alpha;
			tmp=Minimax_Search(nxt,d-1);
			if(cur.alpha<tmp)
			{
				cur.alpha=tmp;
				endcmd=cmd;
			}
			if(cur.alpha>cur.beta)
				return INF;
			if(cmd.first==3)
				nxt.upd(prcmd(3,origin));
			else
				nxt.undo_place(cmd.second.first,cmd.second.second,cmd.first==4);
			cur.hp.pop();
		}
	}
	else
	{
		while(!cur.hp.empty())
		{	
			cmd=cur.decode(cur.hp.top().first);
			nxt.upd(cmd);
			nxt.upd_dis();
			nxt.beta=cur.beta;
			tmp=Minimax_Search(nxt,d-1);
			if(cur.beta>tmp)
			{
				cur.beta=tmp;
				endcmd=cmd;
			}
			if(cur.alpha>cur.beta)
				return -INF;
			if(cmd.first==0)
				nxt.upd(prcmd(0,origin));
			else
				nxt.undo_place(cmd.second.first,cmd.second.second,cmd.first==1);
			cur.hp.pop();
		}
	}
	if(d==d0)
		step=endcmd;
	return cur.side?cur.alpha:cur.beta;
}
std::pair<int, std::pair<int, int> > action(std::pair<int, std::pair<int, int> > loc) {
	/* Your code here */
	cnt=0;
	loc.second.first<<=1;loc.second.second<<=1;
	ST.upd(loc);
	double rate=Minimax_Search(ST,d0);
	std::cerr<<"Best rate:"<<rate<<"\n";
	std::cerr<<"Search cnt:"<<cnt<<"\n";
	
	ST.upd(step);
	//ST.bfs(ST.Ap[0],ST.Ap[1],ai_side?16:0,1);
	std::cerr<<"Plank cnt:"<<ST.Plankcnt<<"\n";
	step.first-=3;
	step.second.first>>=1;
	step.second.second>>=1;
	//ST.print();
	return step;
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
