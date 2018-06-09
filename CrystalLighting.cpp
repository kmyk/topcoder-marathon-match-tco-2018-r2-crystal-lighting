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

constexpr int C_DEAD_CRYSTAL = '8';
constexpr int C_LANTERN = 'L';

/******************************************************************************
 * functions
 ******************************************************************************/

bool check_letter_constraints(string const & board, string const & letters) {
    set<char> x(ALL(board));
    set<char> y(ALL(letters));
    return includes(ALL(y), ALL(x));
}

vector<string> get_neighborhoods(int h, int w, string const & board) {
    vector<string> neighborhoods(h * w);
    REP (y, h) REP (x, w) {
        REP (dir, 4) {
            int ny = y + neighborhood4_y[dir];
            int nx = x + neighborhood4_x[dir];
            if (ny < 0 or h <= ny or nx < 0 or w <= nx) continue;
            neighborhoods[y * w + x] += board[ny * w + nx];
        }
    }
    return neighborhoods;
}

/**
 * @brief replace dead lanterns with C_DEAD_CRYSTAL
 */
string remove_impossible_crystals(int h, int w, string board) {
    assert (check_letter_constraints(board, ".X123456"));
    vector<string> neighborhoods = get_neighborhoods(h, w, board);
    REP (y, h) REP (x, w) {
        char c = board[y * w + x];
        if (isdigit(c)) {
            int cnt = count(ALL(neighborhoods[y * w + x]), C_EMPTY);
            if (cnt < __builtin_popcount(c - '0')) {
                board[y * w + x] = cnt ? C_DEAD_CRYSTAL : C_OBSTACLE;
            }
        }
    }
    return board;
}

/**
 * @brief fill unused areas with C_OBSTACLE
 */
string fill_unused_area(int h, int w, string board) {
    assert (check_letter_constraints(board, ".X1234568"));
    vector<bool> used(h * w);
    REP (z, h * w) if (board[z] == C_OBSTACLE) {
        used[z] = true;
    }
    vector<pair<int, int> > acc;
    bool has_crystal;
    function<void (int, int)> go = [&](int y, int x) {
        char c = board[y * w + x];
        if (isdigit(c) and c != C_DEAD_CRYSTAL) has_crystal = true;
        used[y * w + x] = true;
        acc.emplace_back(y, x);
        REP (dir, 4) {
            int ny = y + neighborhood4_y[dir];
            int nx = x + neighborhood4_x[dir];
            if (ny < 0 or h <= ny or nx < 0 or w <= nx) continue;
            if (not used[ny * w + nx]) {
                go(ny, nx);
            }
        }
    };
    REP (y, h) REP (x, w) if (not used[y * w + x]) {
        acc.clear();
        has_crystal = false;
        go(y, x);
        if (not has_crystal) {
            for (auto pos : acc) {
                int y, x; tie(y, x) = pos;
                board[y * w + x] = C_OBSTACLE;
            }
        }
    }
    return board;
}

vector<pair<int, int> > list_crystals(int h, int w, string const & board) {
    vector<pair<int, int> > crystals;
    REP (y, h) REP (x, w) {
        if (isdigit(board[y * w + x])) {
            crystals.emplace_back(y, x);
        }
    }
    return crystals;
}

int apply_mirror(char mirror, int dir) {
    assert (mirror == C_MIRROR1 or mirror == C_MIRROR2);
    return dir ^ (mirror == C_MIRROR1 ? 1 : 3);
}

pair<int, int> shoot_ray(int h, int w, string const & board, int y, int x, int dir, vector<char> & light) {
    while (true) {
        y += neighborhood4_y[dir];
        x += neighborhood4_x[dir];
        if (y < 0 or h <= y or x < 0 or w <= x) {
            y = x = -1;
            break;
        }
        char c = board[y * w + x];
        if (c == C_EMPTY) {
            light[y * w + x] ^= 1 << dir;
        } else if (c == C_MIRROR1 or c == C_MIRROR2) {
            dir = apply_mirror(c, dir);
        } else {
            break;
        }
    }
    return make_pair(y, x);
}

vector<pair<int, int> > list_ray_sources(int h, int w, string const & board, int y0, int x0, vector<char> const & light) {
    vector<pair<int, int> > results;
    REP (dir, 4) {
        if (not (light[y0 * w + x0] & (1 << dir))) continue;
        int y = y0;
        int x = x0;
        while (true) {
            y -= neighborhood4_y[dir];
            x -= neighborhood4_x[dir];
            char c = board[y * w + x];
            if (c == C_EMPTY) {
                // nop
            } else if (c == C_MIRROR1 or c == C_MIRROR2) {
                dir = (apply_mirror(c, (dir + 2) % 4) + 2) % 4;
            } else {
                break;
            }
        }
        results.emplace_back(y, x);
    }
    return results;
}

/**
 * @description 0bXXBBYYRR: (BB, YY, RR) contains numbers of 3 colors, XX is used if (unique) one of them is 4
 */
uint8_t pack_light(array<char, 3> const & l) {
    if (l[0] == 4 or l[1] == 4 or l[2] == 4) {
        int i = 0;
        while (l[i] != 4) ++ i;
        return (i + 1) << 6;
    } else {
        return l[0] | (l[1] << 2) | (l[2] << 4);
    }
}
array<char, 3> unpack_light(uint8_t l) {
    if (l & 0xc0) {
        array<char, 3> unpacked = {};
        unpacked[(l >> 6) - 1] = 4;
        return unpacked;
    } else {
        char b = l & 0x3;
        char y = (l >> 2) & 0x3;
        char r = (l >> 4) & 0x3;
        return (array<char, 3>) { b, y, r };
    }
}
char apply_light(char letter, char light, int delta) {
    array<char, 3> unpacked = unpack_light(light);
    unpacked[__builtin_ffs(letter - '0') - 1] += delta;
    return pack_light(unpacked);
}
char squash_light(array<char, 3> const & l) {
    return int(bool(l[0])) | (int(bool(l[1])) << 1) | (int(bool(l[2])) << 2);
}

struct result_info_t {
    int score;
    int added_lanterns;
    int added_mirrors;
    int added_obstacles;
    int crystals_primary_ok;
    int crystals_secondary_ok;
    int crystals_incorrect;
    int crystals_secondary_partial;  // subset of incorrect
};
const result_info_t invalid_result = { - 1000000 };

result_info_t compute_result_info(int h, int w, string board, cost_t cost, max_t max_, vector<output_t> const & commands, vector<char> & light) {
    result_info_t info = {};
    for (output_t command : commands) {
        int y, x; char c; tie(y, x, c) = command;
        if (y < 0 or h <= y or x < 0 or w <= x) return invalid_result;  // You can only place items within the board.
        if (board[y * w + x] != C_EMPTY) return invalid_result;  // You can only place items on empty cells of the board. / You can not place two items on the same cell.
        if (c == C_BLUE or c == C_YELLOW or c == C_RED) {
            ++ info.added_lanterns;
            board[y * w + x] = C_LANTERN;
        } else if (c == C_MIRROR1 or c == C_MIRROR2) {
            board[y * w + x] = c;
            ++ info.added_mirrors;
        } else if (c == C_OBSTACLE) {
            board[y * w + x] = c;
            ++ info.added_obstacles;
        } else {
            assert (false);  // Invalid item type
        }
    }
    if (info.added_mirrors > max_.mirrors) return invalid_result;  // You can place at most ??? mirrors.
    if (info.added_obstacles > max_.obstacles) return invalid_result;  // You can place at most ??? obstacles.
    light.assign(h * w, 0);
    for (output_t command : commands) {
        int y, x; char c; tie(y, x, c) = command;
        if (c == C_BLUE or c == C_YELLOW or c == C_RED) {
            REP (dir, 4) {
                int ny, nx; tie(ny, nx) = shoot_ray(h, w, board, y, x, dir, light);
                if (ny == -1) continue;
                if (isdigit(board[ny * w + nx])) {
                    light[ny * w + nx] = apply_light(c, light[ny * w + nx], +1);
                } else if (board[ny * w + nx] == C_LANTERN) {
                    return invalid_result;  // A lantern should not be illuminated by any light ray.
                }
            }
        }
    }
    REP (y, h) REP (x, w) if (isdigit(board[y * w + x])) {
        int l = squash_light(unpack_light(light[y * w + x]));
        if (l) {
            int b = board[y * w + x] - '0';
            if (l == b) {
                ++ (__builtin_popcount(l) == 1 ? info.crystals_primary_ok : info.crystals_secondary_ok);
            } else {
                ++ info.crystals_incorrect;
                if ((b | l) == b) {  // l \subseteq b
                    ++ info.crystals_secondary_partial;
                }
            }
        }
    }
    info.score -= info.added_lanterns * cost.lantern;
    info.score -= info.added_mirrors * cost.mirror;
    info.score -= info.added_obstacles * cost.obstacle;
    info.score += info.crystals_primary_ok * 20;
    info.score += info.crystals_secondary_ok * 30;
    info.score -= info.crystals_incorrect * 10;
    return info;
}

int compute_score(int h, int w, string const & board, cost_t cost, max_t max_, vector<output_t> const & commands) {
    vector<char> light;
    return compute_result_info(h, w, board, cost, max_, commands, light).score;
}


/******************************************************************************
 * the main function
 ******************************************************************************/

vector<output_t> solve(int h, int w, string board, cost_t cost, max_t max_) {
    double clock_begin = rdtsc();
    random_device device;
    xor_shift_128 gen(device());

    // remove unnecessary items
    board = remove_impossible_crystals(h, w, board);
    board = fill_unused_area(h, w, board);
#ifdef LOCAL
    REP (y, h) {
        REP (x, w) {
            cerr << board[y * w + x];
        }
        cerr<< endl;
    }
#endif
    vector<pair<int, int> > crystals = list_crystals(h, w, board);

    // result of SA
    vector<output_t> result;
    int highscore = 0;

    // state of SA
    vector<output_t> cur;
    vector<char> light;
    result_info_t info = compute_result_info(h, w, board, cost, max_, cur, light);

    // misc values
    int iteration = 0;
    double temperature = 1;
    double sa_clock_begin = rdtsc();
    double sa_clock_end = clock_begin + TLE * 0.95;

    auto evaluate = [&](result_info_t const & info) {
        return info.score + max(0.0, temperature - 0.1) * info.crystals_secondary_partial * 40;
    };
    auto try_update = [&]() {
        result_info_t next_info = compute_result_info(h, w, board, cost, max_, cur, light);
        if (highscore < next_info.score) {
            result = cur;
            highscore = next_info.score;
#ifdef LOCAL
            cerr << "highscore = " << highscore << "  (at " << iteration << ", " << temperature << ")" << endl;
#endif
        }
        double evaluated = evaluate(info);
        double next_evaluated = evaluate(next_info);
        int delta = next_evaluated - evaluated;
        if (delta >= 0 or bernoulli_distribution(exp(delta / temperature))(gen)) {
            info = next_info;
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
    int raw_score = compute_score(h, w, board, cost, max_, result);
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
