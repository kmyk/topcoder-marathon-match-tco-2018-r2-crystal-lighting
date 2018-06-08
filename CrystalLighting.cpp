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
 * constants
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

struct cost_t { int lantern, mirror, obstacle; };
struct max_t { int mirrors, obstacles; };


/******************************************************************************
 * functions
 ******************************************************************************/

vector<pair<int, int> > list_crystals(int h, int w, string const & board) {
    vector<pair<int, int> > crystals;
    REP (y, h) REP (x, w) {
        if (isdigit(board[y * w + x])) {
            crystals.emplace_back(y, x);
        }
    }
    return crystals;
}

pair<int, int> shoot_ray(int h, int w, string const & board, int y, int x, int dy, int dx) {
    while (true) {
        y += dy;
        x += dx;
        if (y < 0 or h <= y or x < 0 or w <= x) {
            y = x = -1;
            break;
        }
        if (board[y * w + x] == C_EMPTY) {
            // nop
        } else if (board[y * w + x] == C_MIRROR1) {
            dy *= -1;
            dx *= -1;
            swap(dy, dx);
        } else if (board[y * w + x] == C_MIRROR2) {
            swap(dy, dx);
        } else {
            break;
        }
    }
    return make_pair(y, x);
}

int compute_score(int h, int w, string board, cost_t cost, max_t max_, vector<pair<int, int> > const & crystals, vector<output_t> const & commands) {
    constexpr int C_LANTERN = 'l';
    constexpr int MINUS_INF = - 1000000;
    int count_lanterns = 0;
    int count_mirrors = 0;
    int count_obstacles = 0;
    for (output_t command : commands) {
        int y, x; char c; tie(y, x, c) = command;
        if (y < 0 or h <= y or x < 0 or w <= x) return MINUS_INF;  // You can only place items within the board.
        if (board[y * w + x] != C_EMPTY) return MINUS_INF;  // You can only place items on empty cells of the board. / You can not place two items on the same cell.
        if (c == C_BLUE or c == C_YELLOW or c == C_RED) {
            ++ count_lanterns;
            board[y * w + x] = C_LANTERN;
        } else if (c == C_MIRROR1 or c == C_MIRROR2) {
            board[y * w + x] = c;
            ++ count_mirrors;
        } else if (c == C_OBSTACLE) {
            board[y * w + x] = c;
            ++ count_obstacles;
        } else {
            assert (false);  // Invalid item type
        }
    }
    if (count_mirrors > max_.mirrors) return MINUS_INF;  // You can place at most ??? mirrors.
    if (count_obstacles > max_.obstacles) return MINUS_INF;  // You can place at most ??? obstacles.
    vector<char> lit(h * w);
    for (output_t command : commands) {
        int y, x; char c; tie(y, x, c) = command;
        if (c == C_BLUE or c == C_YELLOW or c == C_RED) {
            REP (i, 4) {
                int ny, nx; tie(ny, nx) = shoot_ray(h, w, board, y, x, neighborhood4_y[i], neighborhood4_x[i]);
                if (ny == -1) continue;
                if (isdigit(board[ny * w + nx])) {
                    lit[ny * w + nx] |= (c - '0');
                } else if (board[ny * w + nx] == C_LANTERN) {
                    return MINUS_INF;  // A lantern should not be illuminated by any light ray.
                }
            }
        }
    }
    int score = 0;
    score -= count_lanterns * cost.lantern;
    score -= count_mirrors * cost.mirror;
    score -= count_obstacles * cost.obstacle;
    for (auto const & pos : crystals) {
        int y, x; tie(y, x) = pos;
        char l = lit[y * w + x];
        if (l) {
            char b = board[y * w + x];
            score += (l == b - '0' ? 10 + 10 * __builtin_popcount(l) : - 10);
        }
    }
    return score;
}


/******************************************************************************
 * the main function
 ******************************************************************************/

vector<output_t> solve(int h, int w, string const & board, cost_t cost, max_t max_) {
    double clock_begin = rdtsc();
    random_device device;
    xor_shift_128 gen(device());

    vector<pair<int, int> > crystals = list_crystals(h, w, board);

    // result of SA
    vector<output_t> result;
    int highscore = 0;

    // state of SA
    vector<output_t> cur;
    int score = 0;

    // misc values
    int iteration = 0;
    double temperature = 1;
    double sa_clock_begin = rdtsc();
    double sa_clock_end = clock_begin + TLE * 0.95;

    auto try_update = [&]() {
        int next_score = compute_score(h, w, board, cost, max_, crystals, cur);
        int delta = next_score - score;
        if (delta >= 0 or bernoulli_distribution(exp(delta / temperature))(gen)) {
            score = next_score;
            if (highscore < score) {
                result = cur;
                highscore = score;
		cerr << "highscore = " << score << "  (at " << iteration << ", " << temperature << ")" << endl;
            }
            return true;
        } else {
            return false;
        }
    };

    for (; ; ++ iteration) {
        if (temperature < 0.1 or iteration % 128 == 0) {
            double t = rdtsc();
            if (t > sa_clock_end) break;
            temperature = 1 - (t - sa_clock_begin) / (sa_clock_end - sa_clock_begin);
        }
        int prob = uniform_int_distribution<int>(0, 99)(gen);
        static const array<char, 6> item_table = { C_BLUE, C_YELLOW, C_RED, C_MIRROR1, C_MIRROR2, C_OBSTACLE };

        if (prob < 50) {  // move one
            if (cur.empty()) continue;
            int i = uniform_int_distribution<int>(0, cur.size() - 1)(gen);
            int dir = uniform_int_distribution<int>(0, 4 - 1)(gen);
            get<0>(cur[i]) += neighborhood4_y[dir];
            get<1>(cur[i]) += neighborhood4_x[dir];
            if (not try_update()) {
                get<0>(cur[i]) -= neighborhood4_y[dir];
                get<1>(cur[i]) -= neighborhood4_x[dir];
            }

        } else if (prob < 60) {  // modify one
            if (cur.empty()) continue;
            int i = uniform_int_distribution<int>(0, cur.size() - 1)(gen);
            char c = item_table[uniform_int_distribution<int>(0, item_table.size() - 1)(gen)];
            swap(get<2>(cur[i]), c);
            if (not try_update()) {
                swap(get<2>(cur[i]), c);
            }

        } else if (prob < 80) {  // remove one
            if (cur.empty()) continue;
            int i = uniform_int_distribution<int>(0, cur.size() - 1)(gen);
            swap(cur[i], cur.back());
            auto preserved = cur.back();
            cur.pop_back();
            if (not try_update()) {
                cur.push_back(preserved);
            }

        } else {  // add one
            int y = uniform_int_distribution<int>(0, h - 1)(gen);
            int x = uniform_int_distribution<int>(0, w - 1)(gen);
            char c = item_table[uniform_int_distribution<int>(0, item_table.size() - 1)(gen)];
            cur.emplace_back(y, x, c);
            if (not try_update()) {
                cur.pop_back();
            }
        }
    }

    ll seed = -1;
#ifdef LOCAL
    if (getenv("SEED")) seed = atoll(getenv("SEED"));
#endif
    int num_empty = count(ALL(board), C_EMPTY);
    int num_obstacles = count(ALL(board), C_OBSTACLE);
    int num_crystals = h * w - num_obstacles - num_empty;
    int raw_score = compute_score(h, w, board, cost, max_, crystals, result);
    double elapsed = rdtsc() - clock_begin;
    cerr << "{\"seed\":" << seed;
    cerr << ",\"H\":" << h;
    cerr << ",\"W\":" << w;
    cerr << ",\"costLantern\":" << cost.lantern;
    cerr << ",\"costMirror\":" << cost.mirror;
    cerr << ",\"costObstacle\":" << cost.obstacle;
    cerr << ",\"maxMirrors\":" << max_.mirrors;
    cerr << ",\"maxObstacles\":" << max_.obstacles;
    cerr << ",\"numEmpty\":" << num_empty;
    cerr << ",\"numObstacles\":" << num_obstacles;
    cerr << ",\"numCrystals\":" << num_crystals;
    cerr << ",\"raw_score\":" << raw_score;
    cerr << ",\"iteration\":" << iteration;
    cerr << ",\"elapsed\":" << elapsed;
    cerr << "}" << endl;
    return result;
}


/******************************************************************************
 * functions to prepare input format
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

string pack_board(vector<string> const & target_board) {
    int h = target_board.size();
    int w = target_board[0].size();
    string packed(h * w, '\0');
    REP (y, h) REP (x, w) packed[y * w + x] = target_board[y][x];
    return packed;
}


/******************************************************************************
 * the entry point
 ******************************************************************************/

class CrystalLighting {
public:
    vector<string> placeItems(vector<string> targetBoard, int costLantern, int costMirror, int costObstacle, int maxMirrors, int maxObstacles) {
        cost_t cost = { costLantern, costMirror, costObstacle };
        max_t max_ = { maxMirrors, maxObstacles };
        vector<output_t> commands =
            with_landscape(targetBoard, [&](vector<string> const & targetBoard) {
                int h = targetBoard.size();
                int w = targetBoard[0].size();
                string packed = pack_board(targetBoard);
                return solve(h, w, packed, cost, max_);
            });
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
