#include "AIController.h"
#include <utility>
#include <cstring>
#include <unordered_map>
#include <vector>
#include <cmath>
#include <stack>
#include <cstdlib>
#include <ctime>
#include <queue>

extern int ai_side;
int Layer = 0;
std::string ai_name = "jbkbvgcf";
//搞一个从当前局面向这个局面相关信息的映射
const unsigned int inf = 1e9 + 9;
enum Direction {
    across,
    vertical
};
enum Turn {
    player_0,
    player_1
};

// ai_side 表示的是我们是哪一边
// Current表示的是现在的棋局长什么样子，
//我们可以从Map找到这个棋局接下来会有的操作，也就相当于通过某种方式达到它的某个儿子局面
//还可以通过Map找到这个棋局的胜利次数以及经过这个点的次数
int Round = 0;
clock_t start;
std::pair<int, int> Last1;
std::pair<int, int> Last2;

struct State {
    int Player_0Row = 8, Player_0Column = 4;
    int Player_1Row = 0, Player_1Column = 4;
    bool Across[8][8];
    bool Vertical[8][8];
    int Player0_Board = 10;
    int Player1_Board = 10;
    Turn Play; //这一局是谁在走

    State() {
        memset(Across, false, sizeof(Across));
        memset(Vertical, false, sizeof(Vertical));
    }

    State(const State &state) {
        Player_0Column = state.Player_0Column;
        Player_0Row = state.Player_0Row;
        Player_1Row = state.Player_1Row;
        Player_1Column = state.Player_1Column;
        for (int i = 0; i < 8; ++i) {
            for (int j = 0; j < 8; ++j) {
                Across[i][j] = state.Across[i][j];
                Vertical[i][j] = state.Vertical[i][j];
            }
        }
        Player0_Board = state.Player0_Board;
        Player1_Board = state.Player1_Board;
        Play = state.Play;
    }

    State &operator=(const State &state) {
        if (this == &state)
            return *this;
        Player_0Column = state.Player_0Column;
        Player_0Row = state.Player_0Row;
        Player_1Row = state.Player_1Row;
        Player_1Column = state.Player_1Column;
        for (int i = 0; i < 8; ++i) {
            for (int j = 0; j < 8; ++j) {
                Across[i][j] = state.Across[i][j];
                Vertical[i][j] = state.Vertical[i][j];
            }
        }
        Player0_Board = state.Player0_Board;
        Player1_Board = state.Player1_Board;
        Play = state.Play;
        return *this;
    }

    bool operator==(const State &rhs) const {
        if (Player_0Row != rhs.Player_0Row)
            return false;
        if (Player_1Row != rhs.Player_1Row)
            return false;
        if (Player_1Column != rhs.Player_1Column)
            return false;
        if (Player_0Column != rhs.Player_0Column)
            return false;
        if (Play != rhs.Play)
            return false;
        for (int i = 0; i < 8; ++i) {
            for (int j = 0; j < 8; ++j) {
                if (Across[i][j] != rhs.Across[i][j])
                    return false;
                if (Vertical[i][j] != rhs.Vertical[i][j])
                    return false;
            }
        }
        return true;
    }

    void Update(const int &Move) {
        Turn turn = Play;
        int operation = Move / 100;
        int row = Move % 100 / 10;
        int column = Move % 10;
        if (operation == 0) {
            if (turn == 0) {
                Player_0Row = row;
                Player_0Column = column;
            } else {
                Player_1Row = row;
                Player_1Column = column;
            }
        } else if (operation == 2) {
            Across[row][column] = true;
            if (turn == 0)
                Player0_Board--;
            else
                Player1_Board--;
        } else if (operation == 1) {
            Vertical[row][column] = true;
            if (turn == 0)
                Player0_Board--;
            else
                Player1_Board--;
        }
        Play = (turn == player_0 ? player_1 : player_0);
    }

    void Print() const {
        if (Play == player_0)
            std::cerr << "player_0" << std::endl;
        else
            std::cerr << "player_1" << std::endl;
        std::cerr << "=================================" << std::endl;
        char board[17][17];
        for (int i = 0; i < 17; ++i) {
            for (int j = 0; j < 17; ++j) {
                if (i % 2 == 1 || j % 2 == 1)
                    board[i][j] = ' ';
                else
                    board[i][j] = '.';
            }
        }
        board[2 * Player_0Row][2 * Player_0Column] = '0';
        board[2 * Player_1Row][2 * Player_1Column] = '1';
        for (int i = 0; i < 8; ++i) {
            for (int j = 0; j < 8; ++j) {
                if (Vertical[i][j]) {
                    board[2 * i + 1][2 * j + 1] = '|';
                    board[2 * i][2 * j + 1] = '|';
                    board[2 * i + 2][2 * j + 1] = '|';
                }
                if (Across[i][j]) {
                    board[2 * i + 1][2 * j + 1] = '=';
                    board[2 * i + 1][2 * j + 2] = '=';
                    board[2 * i + 1][2 * j] = '=';
                }
            }
        }
        for (int i = 0; i < 17; ++i) {
            for (int j = 0; j < 17; ++j) {
                std::cerr << board[i][j] << " ";
            }
            std::cerr << std::endl;
        }
        std::cerr << std::endl;
        std::cerr << "=================================" << std::endl;
    }
};

State Current; //我们有一个现阶段的Current状态，表示的是现在博弈的情况。
struct StateInfo {
    int Nv = 0;    //表示的是这个节点被访问了多少次
    double Qv = 0; //表示的是这个点的胜出次数
    std::vector<int> move[2];
    std::vector<int> explored_move[2];
};

struct HashFunc {
    const int seed_1_v = 7;
    const int seed_2_v = 19;
    const int seed_1_a = 11;
    const int seed_2_a = 13;
    const int seed_3_r = 17;
    const int seed_3_c = 5;
    const int seed_4_r = 3;
    const int seed_4_c = 23;
    const int seed_5_0 = 29;
    const int seed_5_1 = 31;
    const int seed_6 = 2;
    const unsigned int mod = 1e9 + 7;

    size_t operator()(const State &state) const {
        unsigned int res = 0;
        for (int i = 0; i < 8; ++i) {
            for (int j = 0; j < 8; ++j) {
                res += state.Vertical[i][j] *
                       ((((unsigned int) (seed_1_v * i)) % mod + ((unsigned int) (seed_2_v * j)) % mod) % mod);
                res %= mod;
                res += state.Across[i][j] *
                       ((((unsigned int) (seed_1_a * i)) % mod + ((unsigned int) (seed_2_a * j)) % mod) % mod);
                res %= mod;
            }
        }
        res += (((unsigned int) (seed_3_r * state.Player_0Row)) % mod +
                ((unsigned int) (seed_3_c * state.Player_0Column)) % mod) %
               mod;
        res %= mod;
        res += (((unsigned int) (seed_4_r * state.Player_1Row)) % mod +
                ((unsigned int) (seed_4_c * state.Player_1Column)) % mod) %
               mod;
        res %= mod;
        res += ((unsigned int) (seed_5_1 * state.Player1_Board)) % mod;
        res %= mod;
        res += ((unsigned int) (seed_5_0 * state.Player0_Board)) % mod;
        res %= mod;
        res += (int) state.Play * seed_6;
        res %= mod;
        return res;
    }
};

std::unordered_map<State, StateInfo, HashFunc> Map;

void BFS(int Court[9][9], const int &des, const State &state) {
    std::queue<std::pair<int, int>> q;
    for (int i = 0; i < 9; ++i) {
        q.push({des, i});
        Court[des][i] = 1;
    }
    while (!q.empty()) {
        auto from = q.front();
        q.pop();
        if (from.first - 1 >= 0) { //可以向上走一步
            if ((from.second - 1 < 0 || !state.Across[from.first - 1][from.second - 1]) &&
                (from.second >= 8 || !state.Across[from.first - 1][from.second]) &&
                !Court[from.first - 1][from.second]) {
                Court[from.first - 1][from.second] = Court[from.first][from.second] + 1;
                q.push({from.first - 1, from.second});
            }
        }
        if (from.first + 1 <= 8) { //可以往下走一步
            if ((from.second - 1 < 0 || !state.Across[from.first][from.second - 1]) &&
                (from.second >= 8 || !state.Across[from.first][from.second]) &&
                !Court[from.first + 1][from.second]) {
                Court[from.first + 1][from.second] = Court[from.first][from.second] + 1;
                q.push({from.first + 1, from.second});
            }
        }
        if (from.second - 1 >= 0) { //可以往左走一步
            if ((from.first - 1 < 0 || !state.Vertical[from.first - 1][from.second - 1]) &&
                (from.first >= 8 || !state.Vertical[from.first][from.second - 1]) &&
                !Court[from.first][from.second - 1]) {
                Court[from.first][from.second - 1] = Court[from.first][from.second] + 1;
                q.push({from.first, from.second - 1});
            }
        }
        if (from.second + 1 <= 8) { //可以往you走一步
            if ((from.first - 1 < 0 || !state.Vertical[from.first - 1][from.second]) &&
                (from.first >= 8 || !state.Vertical[from.first][from.second]) &&
                !Court[from.first][from.second + 1]) {
                Court[from.first][from.second + 1] = Court[from.first][from.second] + 1;
                q.push({from.first, from.second + 1});
            }
        }
    }
}

//返回的是<我现在的步数，对手先前的步数&对手之后的步数>
//希望知道wanted这方玩家在修改和不修改前后的步数
std::pair<int, std::pair<int, int>>
CheckDistance(State state, const std::pair<int, int> &coordinate, const Direction &dir, const Turn &wanted) {
    int rival_now, rival_origin, self_now;
    int Court[9][9];
    std::queue<std::pair<int, int>> q;
    int self_destination, rival_destination;
    if (wanted == player_0) // wanted是敌手的玩家编号
    {
        self_destination = 8;
        rival_destination = 0;
    } else {
        self_destination = 0;
        rival_destination = 8;
    }
    memset(Court, 0, sizeof(Court));
    BFS(Court, rival_destination, state);
    if (wanted == player_0)
        rival_origin = Court[state.Player_0Row][state.Player_0Column];
    else
        rival_origin = Court[state.Player_1Row][state.Player_1Column];

    if (dir == across)
        state.Across[coordinate.first][coordinate.second] = true;
    else
        state.Vertical[coordinate.first][coordinate.second] = true;
    memset(Court, 0, sizeof(Court));
    BFS(Court, rival_destination, state);
    if (wanted == player_0)
        rival_now = Court[state.Player_0Row][state.Player_0Column];
    else
        rival_now = Court[state.Player_1Row][state.Player_1Column];
    memset(Court, 0, sizeof(Court));
    BFS(Court, self_destination, state);
    if (wanted == player_0)
        self_now = Court[state.Player_1Row][state.Player_1Column];
    else
        self_now = Court[state.Player_0Row][state.Player_0Column];

    return {self_now, {rival_origin, rival_now}};
}

bool CheckBoard(State state, const std::pair<int, int> &coordinate, const Direction &dir, const Turn &turn) {
    if (Round < 20 &&
        ((abs(coordinate.second - state.Player_1Column) >= 2 || abs(coordinate.first - state.Player_1Row) >= 2) &&
         (abs(coordinate.second - state.Player_0Column) >= 2 || abs(coordinate.first - state.Player_0Row) >= 2)))
        return false;
    if (state.Across[coordinate.first][coordinate.second] || state.Vertical[coordinate.first][coordinate.second])
        return false;
    if (dir == vertical) {
        if ((coordinate.second - 1 >= 0 && state.Across[coordinate.first][coordinate.second - 1]) &&
            (coordinate.second + 1 < 8 && state.Across[coordinate.first][coordinate.second + 1]))
            return false;
        if ((coordinate.first + 1 < 8 && state.Vertical[coordinate.first + 1][coordinate.second]) ||
            (coordinate.first - 1 >= 0 && state.Vertical[coordinate.first - 1][coordinate.second]))
            return false;
    }
    if (dir == across) {
        if ((coordinate.first + 1 < 8 && state.Vertical[coordinate.first + 1][coordinate.second]) &&
            (coordinate.first - 1 >= 0 && state.Vertical[coordinate.first - 1][coordinate.second]))
            return false;
        if ((coordinate.second - 1 >= 0 && state.Across[coordinate.first][coordinate.second - 1]) ||
            (coordinate.second + 1 < 8 && state.Across[coordinate.first][coordinate.second + 1]))
            return false;
    }

    Turn rival = (turn == player_0 ? player_1 : player_0);
    auto p = CheckDistance(state, coordinate, dir, rival);
    if (p.first == 0 || p.second.second == 0)
        return false; //如果说让对手到不了了肯定是不行的
    return p.second.first < p.second.second;
}

double Estimate(const State &state, const Turn &turn) {
    int Court[9][9];
    memset(Court, 0, sizeof(Court));
    BFS(Court, 0, state);
    int d0 = Court[state.Player_0Row][state.Player_0Column];
    memset(Court, 0, sizeof(Court));
    BFS(Court, 8, state);
    int d1 = Court[state.Player_1Row][state.Player_1Column];
//    state.Print();
//    std::cerr << "d0 = " << d0 << std::endl;
//    std::cerr << "d1 = " << d1 << std::endl;
    if (turn == player_0) {//player_0赢的胜率
        //d1越大，我现在手里剩下的板越多，我越容易赢
        return ((double) d1 + 0.3 * state.Player0_Board) /
               ((double) (d1 + d0) + 0.3 * (state.Player0_Board + state.Player1_Board));
    } else {
        return ((double) d0 + 0.3 * state.Player1_Board) /
               ((double) (d1 + d0) + 0.3 * (state.Player0_Board + state.Player1_Board));
    }
}

int Rand(const std::vector<int> &move) {
    //返回的是操作的index
    int move_step = 0;
    for (auto ptr = move.begin(); *ptr / 100 == 0 && ptr != move.end(); ++ptr)
        move_step++;
    if (move_step == move.size())
        return rand() % move.size();

    int size = move.size() + move_step * 5;
    int random_index = rand() % size;
    if (random_index <= move_step * 5) {
        if (move[random_index / 5] % 100 / 10 == Last2.first && move[random_index / 5] % 10 == Last2.second)
            return rand() % move.size() / 5;
        else
            return random_index / 5;
    } else
        return random_index - move_step * 5;
}

void Enumerate(const State &state, const Turn &turn, std::vector<int> &move) {
    //    state.Print();
    int Row, Column;
    int OtherRow, OtherColumn;
    if (turn == player_0) {
        Row = state.Player_0Row;
        Column = state.Player_0Column;
        OtherRow = state.Player_1Row;
        OtherColumn = state.Player_1Column;
    } else {
        Row = state.Player_1Row;
        Column = state.Player_1Column;
        OtherRow = state.Player_0Row;
        OtherColumn = state.Player_0Column;
    }
    //先考虑走一步
    move.clear();
    int row, column, operation = 0;
    //往上走一步，可能的所有情况
    //如果说对手棋子正好在我方棋子上方
    //我要检查我前面是否有板子，并且检查板子的时候还得注意不能越过边界检查

    if ((Column - 1 < 0 || !state.Across[Row - 1][Column - 1]) &&
        (Column >= 8 || !state.Across[Row - 1][Column])) {
        if (OtherColumn == Column &&
            OtherRow == Row - 1) {
            if (Row - 2 >= 0) { //可以隔过这个点走两个
                if ((Column - 1 < 0 || !state.Across[Row - 2][Column - 1]) &&
                    (Column >= 8 || !state.Across[Row - 2][Column])) {
                    row = Row - 2;
                    column = Column;
                    // std::cerr << operation << " " << row << " " << column << std::endl;
                    move.push_back(operation * 100 + row * 10 + column);
                } else {
                    //就是说两个格子之后会有板子挡着
                    if (Column - 1 >= 0 && !state.Vertical[Row - 1][Column - 1] &&
                        (Row - 2 < 0 || !state.Vertical[Row - 2][Column - 1])) {
                        row = Row - 1;
                        column = Column - 1;
                        move.push_back(operation * 100 + row * 10 + column);
                    }
                    if (Column < 8 && !state.Vertical[Row - 1][Column] &&
                        (Row - 2 < 0 || !state.Vertical[Row - 2][Column])) {
                        row = Row - 1;
                        column = Column + 1;
                        move.push_back(operation * 100 + row * 10 + column);
                    }
                }
            } else {
                if (Column - 1 >= 0 && !state.Vertical[Row - 1][Column - 1] &&
                    (Row - 2 < 0 || !state.Vertical[Row - 2][Column - 1])) {
                    row = Row - 1;
                    column = Column - 1;
                    move.push_back(operation * 100 + row * 10 + column);
                }
                if (Column < 8 && !state.Vertical[Row - 1][Column] &&
                    (Row - 2 < 0 || !state.Vertical[Row - 2][Column])) {
                    row = Row - 1;
                    column = Column + 1;
                    move.push_back(operation * 100 + row * 10 + column);
                }
            }
        }
            //如果我的正前方没有板也没有点，就可以往正前方走一步
        else if (Row - 1 >= 0) {
            row = Row - 1;
            column = Column;
            move.push_back(operation * 100 + row * 10 + column);
        }
    }

    //往下走一步所有可能的情况
    //如果说对手棋子正好在我方棋子下方
    //先要检查我方棋子下前是没有板子的
    if ((Column - 1 < 0 || !state.Across[Row][Column - 1]) &&
        (Column >= 8 || !state.Across[Row][Column])) {
        if (OtherColumn == Column &&
            OtherRow == Row + 1) {
            if (Row + 2 <= 8) { //可以一下跳两个没到边界
                if ((Column - 1 < 0 || !state.Across[Row + 1][Column - 1]) &&
                    (Column >= 8 || !state.Across[Row + 1][Column])) {
                    row = Row + 2;
                    column = Column;
                    move.push_back(operation * 100 + row * 10 + column);
                } else {
                    //就说明下面至少有一个地方有板子挡着
                    if (Column - 1 >= 0 && !state.Vertical[Row][Column - 1] &&
                        (Row + 1 >= 8 || !state.Vertical[Row + 1][Column - 1])) {
                        row = Row + 1;
                        column = Column - 1;
                        move.push_back(operation * 100 + row * 10 + column);
                    }
                    if (Column < 8 && !state.Vertical[Row][Column] &&
                        (Row + 1 >= 8 || !state.Vertical[Row + 1][Column])) {
                        row = Row + 1;
                        column = Column + 1;
                        move.push_back(operation * 100 + row * 10 + column);
                    }
                }
            } else {
                //否则就说明已经走到边界，不能一下子跳两个格子
                if (Column - 1 >= 0 && !state.Vertical[Row][Column - 1] &&
                    (Row + 1 >= 8 || !state.Vertical[Row + 1][Column - 1])) {
                    row = Row + 1;
                    column = Column - 1;
                    move.push_back(operation * 100 + row * 10 + column);
                }
                if (Column < 8 && !state.Vertical[Row][Column] &&
                    (Row + 1 >= 8 || !state.Vertical[Row + 1][Column])) {
                    row = Row + 1;
                    column = Column + 1;
                    move.push_back(operation * 100 + row * 10 + column);
                }
            }
        }
            //如果我的正前方没有板也没有点，就可以往正前方走一步
        else if (Row + 1 <= 8) {
            row = Row + 1;
            column = Column;
            move.push_back(operation * 100 + row * 10 + column);
        }
    }

    //往右走一步所有可能的情况
    //首先要保证可以往右边走，并且右边没有板
    if (Column < 8 &&
        (Row - 1 < 0 || !state.Vertical[Row - 1][Column]) &&
        (Row >= 8 || !state.Vertical[Row][Column])) {
        //如果说对手棋子正好在我方棋子右方
        if (OtherRow == Row &&
            OtherColumn == Column + 1) {
            if (Column < 7) { //这是说明可以连续走两个
                if ((Row - 1 < 0 || !state.Vertical[Row - 1][Column + 1]) &&
                    (Row >= 8 || !state.Vertical[Row][Column + 1])) {
                    //说明没有板子挡着
                    row = Row;
                    column = Column + 2;
                    move.push_back(operation * 100 + row * 10 + column);
                } else {
                    if (Row - 1 >= 0 && !state.Across[Row - 1][Column] &&
                        (Column + 1 >= 8 || !state.Across[Row - 1][Column + 1])) {
                        if (Column + 1 < 9) {
                            row = Row - 1;
                            column = Column + 1;
                            move.push_back(operation * 100 + row * 10 + column);
                        }
                    }
                    if (Row < 8 && !state.Across[Row][Column] &&
                        (Column + 1 >= 8 || !state.Across[Row][Column + 1])) {
                        if (Column + 1 < 9) {
                            row = Row + 1;
                            column = Column + 1;
                            move.push_back(operation * 100 + row * 10 + column);
                        }
                    }
                }
            } else {
                //只能走一个，不能连续走两个，否则就越界了
                if (Row - 1 >= 0 && !state.Across[Row - 1][Column] &&
                    (Column + 1 >= 8 || !state.Across[Row - 1][Column + 1])) {
                    if (Column + 1 < 9) {
                        row = Row - 1;
                        column = Column + 1;
                        move.push_back(operation * 100 + row * 10 + column);
                    }
                }
                if (Row < 8 && !state.Across[Row][Column] &&
                    (Column + 1 >= 8 || !state.Across[Row][Column + 1])) {
                    if (Column + 1 < 9) {
                        row = Row + 1;
                        column = Column + 1;
                        move.push_back(operation * 100 + row * 10 + column);
                    }
                }
            }
        }
            //如果我的右方没有板，就可以往右方走一步
        else if (Column + 1 <= 8) {
            row = Row;
            column = Column + 1;
            move.push_back(operation * 100 + row * 10 + column);
        }
    }

    //往左走一步所有可能的情况
    //先确定可以往左走，不会有板挡着
    if (Column >= 1 &&
        (Row - 1 < 0 || !state.Vertical[Row - 1][Column - 1]) &&
        (Row >= 8 || !state.Vertical[Row][Column - 1])) {
        //如果说对手棋子正好在我方棋子左方
        if (OtherColumn == Column - 1 &&
            OtherRow == Row) {
            if (Column - 2 >= 0) {
                if ((Row - 1 < 0 || !state.Vertical[Row - 1][Column - 2]) &&
                    (Row >= 8 || !state.Vertical[Row][Column - 2])) {
                    row = Row;
                    column = Column - 2;
                    move.push_back(operation * 100 + row * 10 + column);
                } else {
                    if (Row < 8 && !state.Across[Row][Column - 1] &&
                        (Column - 2 < 0 || !state.Across[Row][Column - 2])) {
                        row = Row + 1;
                        column = Column - 1;
                        move.push_back(operation * 100 + row * 10 + column);
                    }
                    if (Row - 1 >= 0 && !state.Across[Row - 1][Column - 1] &&
                        (Column - 2 < 0 || !state.Across[Row - 1][Column - 2])) {
                        row = Row - 1;
                        column = Column - 1;
                        move.push_back(operation * 100 + row * 10 + column);
                    }
                }
            } else {
                if (Row < 8 && !state.Across[Row][Column - 1] &&
                    (Column - 2 < 0 || !state.Across[Row][Column - 2])) {
                    row = Row + 1;
                    column = Column - 1;
                    move.push_back(operation * 100 + row * 10 + column);
                }
                if (Row - 1 >= 0 && !state.Across[Row - 1][Column - 1] &&
                    (Column - 2 < 0 || !state.Across[Row - 1][Column - 2])) {
                    row = Row - 1;
                    column = Column - 1;
                    move.push_back(operation * 100 + row * 10 + column);
                }
            }
        }
            //如果我的左方没有板，就可以往左方走一步
        else if (Column - 1 >= 0) {
            row = Row;
            column = Column - 1;
            move.push_back(operation * 100 + row * 10 + column);
        }
    }

    int Court[9][9];
    memset(Court, 0, sizeof(Court));
    BFS(Court, (turn == player_0 ? 0 : 8), state);
    int min_ = 81;
    std::vector<int> tmp;
    for (auto ptr = move.begin(); ptr != move.end(); ++ptr) {
        if (Court[*ptr % 100 / 10][(*ptr) % 10] < min_) {
            min_ = Court[*ptr % 100 / 10][(*ptr) % 10];
            tmp.push_back(*ptr);
        }
    }
    move.clear();
    for (auto ptr = tmp.begin(); ptr != tmp.end(); ++ptr) {
        if (Court[*ptr % 100 / 10][(*ptr) % 10] == min_) {
            move.push_back(*ptr);
        }
    }
    //讨论在哪里放板是合适的
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            if ((turn == player_0 && state.Player0_Board > 0) ||
                (turn == player_1 && state.Player1_Board > 0)) {
                if (CheckBoard(state, {i, j}, across, turn)) {
                    operation = 2;
                    move.push_back(operation * 100 + i * 10 + j);
                }
            }
            if ((turn == player_0 && state.Player0_Board > 0) ||
                (turn == player_1 && state.Player1_Board > 0)) {
                if (CheckBoard(state, {i, j}, vertical, turn)) {
                    operation = 1;
                    move.push_back(operation * 100 + i * 10 + j);
                }
            }
        }
    }
    if (move.empty()) {
        // state.Print();
        std::cerr << "Warning" << std::endl;
    }
    //    std::cerr<<move.size()<<std::endl;
}

// init function is called once at the beginning
void init() {
    srand(time(0));
    if (ai_side == 0) {
        Last1 = {8, 4};
        Last2 = {8, 4};
    } else {
        Last1 = {0, 4};
        Last2 = {0, 4};
    }
    Current.Play = (ai_side == 0 ? player_1 : player_0);
}


//根据现在的Current局面选一个子节点作为探索
void Selection() {
    StateInfo &tmp = Map[Current];
    if (tmp.move[(int) Current.Play].empty() && tmp.explored_move[(int) Current.Play].empty()) {
        Enumerate(Current, (Turn) ai_side, Map[Current].move[ai_side]);
    }
    int layer = 0;
    StateInfo info;
    //现在info的move里面已经放好了从Current往下有可能的所有情况
    //每次expand到一个节点就会把它的State数字从move挪到explored_move
    std::stack<State> stack;
    double res;
    State current(Current);
    // stack.push(current);
    while (1) {
        if ((double) (clock() - start) / 1000 > 1949) {
            //std::cerr << "cut off and didn't come to a good node" << std::endl;
            return;
        }
        if (!Map[current].move[current.Play].empty()) { //还没有完全扩展，还有儿子节点可以继续访问
            //下面就在没有expand过的儿子中选几个作为下一个simulation的开始节点
            stack.push(current);
            StateInfo &cur = Map[current];
            int random_index = Rand(cur.move[current.Play]);
            int to_explore = cur.move[current.Play][random_index];
            cur.explored_move[current.Play].push_back(to_explore);
            cur.move[current.Play].erase(cur.move[current.Play].begin() + random_index);
            current.Update(to_explore);
            //我评价现在这个局面，那么应该说现在这个局面揍成这样是上一步人的优或者是劣
            //比如说。刚刚current update的是player0，那么现在这一步是current.play==player1
            //但是我评现在这个局面评的是player0
            //res = Estimate(current, current.Play);就不对了，应该把play翻一下
            res = Estimate(current, (current.Play == player_0 ? player_1 : player_0));
            //std::cerr << "Estimate " << to_explore << " result " << res << std::endl;
            Map[current].Qv += res;
            Map[current].Nv++;
            break;
        } else {
            layer++;
            info = Map[current];
            auto ptr_max = info.explored_move[current.Play].begin();
            double maxUCB = 0;
            //接下来是去找到所有的儿子中UCB最大的
            //先把这个目前状况进栈，等下要更新
            stack.push(current);
            int fatherNv = Map[current].Nv;
            for (auto ptr = info.explored_move[current.Play].begin();
                 ptr != info.explored_move[current.Play].end(); ++ptr) {
                State tmp = current;
                tmp.Update(*ptr); //成为它的一个子节点
                StateInfo info = Map[tmp];
                double UCB;
                if (info.Nv == 0)UCB = inf;
                else
                    UCB = info.Qv / info.Nv +
                          0.05 * sqrt(log(fatherNv) / (double) info.Nv);
                //std::cerr << UCB << "\t";
                if (UCB > maxUCB) {
                    ptr_max = ptr;
                    maxUCB = UCB;
                }
            }
            //std::cerr << std::endl;
//            for (auto ptr = info.explored_move[current.Play].begin();
//                 ptr != info.explored_move[current.Play].end(); ++ptr) {
//                State tmp = current;
//                tmp.Update(*ptr); //成为它的一个子节点
//                StateInfo info = Map[tmp];
//                std::cerr << info.Nv << "\t";
//            }
//            std::cerr << std::endl;
            current.Update(*ptr_max);
            StateInfo &tmp = Map[current];
            if (tmp.move[(int) current.Play].empty() && tmp.explored_move[(int) current.Play].empty()) {
                Enumerate(current, current.Play, Map[current].move[current.Play]);
            }
            //这时的current已经换成了儿子节点中ucb最大的那一个
        }
    }
    // std::cerr << "Q add = " << Map[current].Qv - Q_0 << std::endl;
    //  std::cerr << "N add = " << Map[current].Nv - N_0 << std::endl;
    //  std::cerr << "selected " << N_addition << " nodes" << std::endl;
    while (!stack.empty()) {
        State top = stack.top();
        stack.pop();
        //刚才那一步是current.Play //todo 翻了一下的那一方正在走
        //也就是刚才的res评价的结果呢，如果是current.play就应该加上 1-res
        //如果是正好就是current.play翻了一下，就是res
        if (top.Play == current.Play)
            Map[top].Qv += res; //一共跑了这莫多局，每一局都是1-p
        else
            Map[top].Qv += 1 - res;
        Map[top].Nv++;
    }
    if (layer > Layer)
        Layer = layer;
}

std::pair<int, std::pair<int, int>> action(std::pair<int, std::pair<int, int>> loc) {
    Round++;
    start = clock();
    //传进来一个action，传出去一个action
    if (loc.first >= 0)
        Current.Update(loc.first * 100 + loc.second.first * 10 + loc.second.second);
    else
        Current.Play = (Current.Play == player_0 ? player_1 : player_0);
    int count = 0;
    while ((double) (clock() - start) / 1000 < 1949) {
        count++;
        Selection();
    }
    std::cerr << "layer " << Layer << std::endl;
    Layer = 0;
    std::cerr << "selection " << count << std::endl;
    auto move = Map[Current].explored_move[ai_side];
    auto ptr_max = move.begin();
    int N_max = 0;
    for (auto ptr = move.begin(); ptr != move.end(); ++ptr) {
        // std::cerr << "Looking for the max N ";
        State tmp = Current;
        tmp.Update(*ptr);
        StateInfo &info = Map[tmp];
        //std::cerr << info.Nv << " ";
        if (info.Nv > N_max) {
            N_max = info.Nv;
            ptr_max = ptr;
        }
    }
    //std::cerr << std::endl;
    Current.Update(*ptr_max);
    if (*ptr_max / 100 == 0) {
        Last2 = Last1;
        Last1 = {*ptr_max % 100 / 10, *ptr_max % 10};
    }
    // std::cerr << "current state" << std::endl;
    //    Current.Print();
    clock_t end = clock();
    std::cerr << "$$ " << (double) (end - start) / 1000 << " $$" << std::endl;
    return {*ptr_max / 100, {*ptr_max % 100 / 10, *ptr_max % 10}};
}