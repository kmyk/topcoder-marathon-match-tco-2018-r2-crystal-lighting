#include <bits/stdc++.h>
#define REP(i, n) for (int i = 0; (i) < int(n); ++ (i))
#define REP3(i, m, n) for (int i = (m); (i) < int(n); ++ (i))
#define REP_R(i, n) for (int i = int(n) - 1; (i) >= 0; -- (i))
#define REP3R(i, m, n) for (int i = int(n) - 1; (i) >= int(m); -- (i))
#define ALL(x) begin(x), end(x)
using ll = long long;
using namespace std;
template <class T> using reversed_priority_queue = priority_queue<T, vector<T>, greater<T> >;
template <class T> inline void chmax(T & a, T const & b) { a = max(a, b); }
template <class T> inline void chmin(T & a, T const & b) { a = min(a, b); }
template <typename T> ostream & operator << (ostream & out, vector<T> const & xs) { REP (i, int(xs.size()) - 1) out << xs[i] << ' '; if (not xs.empty()) out << xs.back(); return out; }


/******************************************************************************
 * general libraries
 ******************************************************************************/

constexpr double ticks_per_sec = 2800000000;
constexpr double ticks_per_sec_inv = 1.0 / ticks_per_sec;
inline double rdtsc() { // in seconds
    uint32_t lo, hi;
    asm volatile ("rdtsc" : "=a" (lo), "=d" (hi));
    return (((uint64_t)hi << 32) | lo) * ticks_per_sec_inv;
}
constexpr int TLE = 10; // sec

class xor_shift_128 {
public:
    typedef uint32_t result_type;
    xor_shift_128(uint32_t seed = 42) {
        set_seed(seed);
    }
    void set_seed(uint32_t seed) {
        a = seed = 1812433253u * (seed ^ (seed >> 30));
        b = seed = 1812433253u * (seed ^ (seed >> 30)) + 1;
        c = seed = 1812433253u * (seed ^ (seed >> 30)) + 2;
        d = seed = 1812433253u * (seed ^ (seed >> 30)) + 3;
    }
    uint32_t operator() () {
        uint32_t t = (a ^ (a << 11));
        a = b; b = c; c = d;
        return d = (d ^ (d >> 19)) ^ (t ^ (t >> 8));
    }
    static constexpr uint32_t max() { return numeric_limits<result_type>::max(); }
    static constexpr uint32_t min() { return numeric_limits<result_type>::min(); }
private:
    uint32_t a, b, c, d;
};

const int neighborhood4_y[] = { 0, -1,  0, 1 };
const int neighborhood4_x[] = { 1,  0, -1, 0 };
const int neighborhood8_y[] = { 0, -1, -1, -1, 0, 1, 1, 1 };
const int neighborhood8_x[] = { 1, 1, 0, -1, -1, -1, 0, 1 };


/******************************************************************************
 * utilities
 ******************************************************************************/

 constexpr int MAX_H = 100;
 constexpr int MAX_W = 100;

 enum {
     C_BLUE = '1',
     C_YELLOW = '2',
     C_GREEN = '3',
     C_RED = '4',
     C_VIOLET = '5',
     C_ORANGE = '6',
     C_EMPTY = '.',
     C_MIRROR1 = '/',
     C_MIRROR2 = '\\',
     C_OBSTACLE = 'X',
 };

typedef tuple<int, int, char> output_t;


/******************************************************************************
 * functions to prepare things
 ******************************************************************************/

vector<string> flip_board(vector<string> const & f) {
    int h = f.size();
    int w = f[0].size();
    vector<string> g(w, string(h, '\0'));
    REP (x, w) REP (y, h) {
        g[x][y] = f[y][x];
    }
    return g;
}

vector<output_t> with_landscape(vector<string> const & target_board, function<vector<output_t> (vector<string> const &)> const & cont) {
    int h = target_board.size();
    int w = target_board[0].size();
    if (h <= w) {
        return cont(target_board);
    } else {
        vector<output_t> commands = cont(flip_board(target_board));
        for (output_t & command : commands) {
            swap(get<0>(command), get<1>(command));
        }
        return commands;
    }
}

pair<int, int> shoot_ray(vector<string> const & board, int y, int x, int dy, int dx) {
    int h = board.size();
    int w = board[0].size();
    while (true) {
        y += dy;
        x += dx;
        if (y < 0 or h <= y or x < 0 or w <= x) {
            y = x = -1;
            break;
        }
        if (board[y][x] == C_EMPTY) {
            // nop
        } else if (board[y][x] == C_MIRROR1) {
            dy *= -1;
            dx *= -1;
            swap(dy, dx);
        } else if (board[y][x] == C_MIRROR2) {
            swap(dy, dx);
        } else {
            break;
        }
    }
    return make_pair(y, x);
}

int compute_score(vector<string> board, int cost_lantern, int cost_mirror, int cost_obstacle, vector<output_t> const & commands) {
    int h = board.size();
    int w = board[0].size();
    int acc = 0;
    for (output_t command : commands) {
        int y, x; char c; tie(y, x, c) = command;
        if (c == C_BLUE or c == C_YELLOW or c == C_RED) {
            board[y][x] = 'l';
            acc -= cost_lantern;
        } else if (c == C_MIRROR1 or c == C_MIRROR2) {
            board[y][x] = c;
            acc -= cost_mirror;
        } else if (c == C_OBSTACLE) {
            board[y][x] = c;
            acc -= cost_obstacle;
        } else {
            assert (false);
        }
    }
    vector<char> lit(h * w);
    for (output_t command : commands) {
        int y, x; char c; tie(y, x, c) = command;
        if (c == C_BLUE or c == C_YELLOW or c == C_RED) {
            REP (i, 4) {
                int ny, nx; tie(ny, nx) = shoot_ray(board, y, x, neighborhood4_y[i], neighborhood4_x[i]);
                if (ny == -1) continue;
                if (isdigit(board[ny][nx])) {
                    lit[ny * w + nx] |= (c - '0');
                } else if (board[ny][nx] == 'l') {
                    return - 1000000;  // A lantern should not be illuminated by any light ray.
                }
            }
        }
    }
    REP (y, h) REP (x, w) if (isdigit(board[y][x])) {
        char l = lit[y * w + x];
        if (l) {
            acc += (l == board[y][x] - '0' ? 10 + 10 * __builtin_popcount(l) : - 10);
        }
    }
    return acc;
}


/******************************************************************************
 * the main function
 ******************************************************************************/

vector<output_t> solve(vector<string> const & target_board, int cost_lantern, int cost_mirror, int cost_obstacle, int max_mirrors, int max_obstacles) {
    double clock_begin = rdtsc();
    random_device device;
    xor_shift_128 gen(device());

    return with_landscape(target_board, [&](vector<string> const & target_board) {
        int h = target_board.size();
        int w = target_board[0].size();

        vector<output_t> commands;
        int count_mirrors = 0;
        int count_obstacles = 0;
        int score = 0;
        REP (y, h) REP (x, w) if (target_board[y][x] == C_EMPTY) {
            for (char c : { C_MIRROR1, C_MIRROR2, C_OBSTACLE, C_BLUE, C_YELLOW, C_RED }) {
                if ((c == C_MIRROR1 or c == C_MIRROR2) and count_mirrors == max_mirrors) continue;
                if (c == C_OBSTACLE and count_obstacles == max_obstacles) continue;
                commands.emplace_back(y, x, c);
                int next_score = compute_score(target_board, cost_lantern, cost_mirror, cost_obstacle, commands);
                if (score < next_score) {
                    score = next_score;
                    if (c == C_MIRROR1 or c == C_MIRROR2) ++ count_mirrors;
                    if (c == C_OBSTACLE) ++ count_obstacles;
                    break;
                }
                commands.pop_back();
            }
        }

        ll seed = -1;
#ifdef LOCAL
        if (getenv("SEED")) seed = atoll(getenv("SEED"));
#endif
        int raw_score = compute_score(target_board, cost_lantern, cost_mirror, cost_obstacle, commands);
        double elapsed = rdtsc() - clock_begin;
        cerr << "{\"seed\":" << seed;
        cerr << ",\"raw_score\":" << raw_score;
        cerr << ",\"elapsed\":" << elapsed;
        cerr << "}" << endl;
        return commands;
    });
}


/******************************************************************************
 * the entry point
 ******************************************************************************/

class CrystalLighting {
public:
    vector<string> placeItems(vector<string> targetBoard, int costLantern, int costMirror, int costObstacle, int maxMirrors, int maxObstacles) {
        vector<output_t> commands = solve(targetBoard, costLantern, costMirror, costObstacle, maxMirrors, maxObstacles);
        vector<string> answer;
        for (auto command : commands) {
            int y, x; char c; tie(y, x, c) = command;
            ostringstream oss;
            oss << y << ' ' << x << ' ' << c;
            answer.push_back(oss.str());
        }
        return answer;
    }
};