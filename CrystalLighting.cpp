#pragma GCC optimize "O3"
#pragma GCC target "sse4.2"
#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <cstdio>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <queue>
#include <random>
#include <set>
#include <sstream>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>
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
 * parameters
 ******************************************************************************/

#ifndef BOLTZMANN_1
#define BOLTZMANN_1 (0.1736)
#endif
#ifndef BOLTZMANN_2
#define BOLTZMANN_2 (0.0)
#endif
#ifndef EVAL_PARAM_1
#define EVAL_PARAM_1 (2)
#endif
#ifndef EVAL_PARAM_2
#define EVAL_PARAM_2 (15)
#endif
#ifndef EVAL_PARAM_3
#define EVAL_PARAM_3 (4)
#endif
#ifndef EVAL_PARAM_4
#define EVAL_PARAM_4 (0.03)
#endif
#ifndef NBHD_PROB_1
#define NBHD_PROB_1 (50)
#endif
#ifndef NBHD_PROB_2
#define NBHD_PROB_2 (10)
#endif
#ifndef NBHD_PROB_3
#define NBHD_PROB_3 (10)
#endif
#ifndef NBHD_PROB_4
#define NBHD_PROB_4 (30)
#endif


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

template <class RandomEngine>
int get_random_lt(int n, RandomEngine & gen) {
    return uniform_int_distribution<int>(0, n - 1)(gen);
}
template <class Container, class RandomEngine>
typename Container::value_type choose_random(Container const & xs, RandomEngine & gen) {
    return xs[uniform_int_distribution<int>(0, xs.size() - 1)(gen)];
}


/******************************************************************************
 * constants
 ******************************************************************************/

constexpr int MAX_H = 100;
constexpr int MAX_W = 100;

typedef char letter_t;
enum : char {
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

typedef int color_t;
enum {
    I_BLUE = 1,
    I_YELLOW = 2,
    I_GREEN = 3,
    I_RED = 4,
    I_VIOLET = 5,
    I_ORANGE = 6,
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

vector<pair<int, int> > list_cells_with_letters(int h, int w, string const & board, string const & letters) {
    vector<pair<int, int> > acc;
    acc.reserve(h * w);
    REP (y, h) REP (x, w) {
        if (count(ALL(letters), board[y * w + x])) {
            acc.emplace_back(y, x);
        }
    }
    acc.shrink_to_fit();
    return acc;
}

int apply_mirror(char mirror, int dir) {
    assert (mirror == C_MIRROR1 or mirror == C_MIRROR2);
    return dir ^ (mirror == C_MIRROR1 ? 1 : 3);
}

tuple<int, int, int> chase_ray_source(int h, int w, string const & board, int y, int x, int dir, vector<uint8_t> const & light) {
    assert (light[y * w + x] & (0x3 << (2 * dir)));
    while (true) {
        y -= neighborhood4_y[dir];
        x -= neighborhood4_x[dir];
        letter_t c = board[y * w + x];
        if (c == C_EMPTY) {
            // nop
        } else if (c == C_MIRROR1 or c == C_MIRROR2) {
            dir = (apply_mirror(c, (dir + 2) % 4) + 2) % 4;
        } else {
            assert (c == C_LANTERN);
            break;
        }
    }
    return make_tuple(y, x, dir);
}

vector<output_t> get_commands_from_points(int h, int w, string const & board, vector<uint8_t> const & light, vector<pair<int, int> > const & points) {
    int n = points.size();
    vector<output_t> commands(n);
    REP (i, n) {
        int y, x; tie(y, x) = points[i];
        char c = board[y * w + x];
        commands[i] = make_tuple(y, x, c == C_LANTERN ? (light[y * w + x] >> 4) + '0' : c);
    }
    return commands;
}

color_t summarize_light(uint8_t l) {
    color_t c = 0;
    if (l & 0x03) c |= 1 << ( (l & 0x03)       - 1);
    if (l & 0x0c) c |= 1 << (((l & 0x0c) >> 2) - 1);
    if (l & 0x30) c |= 1 << (((l & 0x30) >> 4) - 1);
    if (l & 0xc0) c |= 1 << (((l & 0xc0) >> 6) - 1);
    return c;
}

struct result_info_t {
    int score;
    int added_lanterns;
    int added_mirrors;
    int added_obstacles;
    int crystals_primary_ok;
    int crystals_secondary_ok;
    int crystals_incorrect;
    int crystals_incorrect_primary_extra_1;
    int crystals_incorrect_primary_extra_2;
    int crystals_incorrect_secondary_half;
    int crystals_incorrect_secondary_extra;
    int lit_lanterns;
    int lit_count;
};
const result_info_t invalid_result = { - 1000000 };

bool operator == (result_info_t const & a, result_info_t const & b) {
    return a.score == b.score
        and a.added_lanterns == b.added_lanterns
        and a.added_mirrors == b.added_mirrors
        and a.added_obstacles == b.added_obstacles
        and a.crystals_primary_ok == b.crystals_primary_ok
        and a.crystals_secondary_ok == b.crystals_secondary_ok
        and a.crystals_incorrect == b.crystals_incorrect
        and a.crystals_incorrect_primary_extra_1 == b.crystals_incorrect_primary_extra_1
        and a.crystals_incorrect_primary_extra_2 == b.crystals_incorrect_primary_extra_2
        and a.crystals_incorrect_secondary_half == b.crystals_incorrect_secondary_half
        and a.crystals_incorrect_secondary_extra == b.crystals_incorrect_secondary_extra
        and a.lit_lanterns == b.lit_lanterns
        and a.lit_count == b.lit_count
        ;
}

void update_score_info_crystal(result_info_t & info, color_t crystal, color_t light, int delta) {
#ifdef LOCAL
    assert (0 <= crystal and crystal <= 8);
    assert (0 <= light and light < 8);
#endif
    if (light) {
        if (light == crystal) {
            (__builtin_popcount(light) == 1 ? info.crystals_primary_ok : info.crystals_secondary_ok) += delta;
        } else {
            info.crystals_incorrect += delta;
            if (crystal == C_DEAD_CRYSTAL - '0') {
                // nop
            } else if ((crystal | light) == crystal) {  // light \subseteq crystal
                info.crystals_incorrect_secondary_half += delta;
            } else if ((light | crystal) == light) {  // crystal \subseteq light
                if (__builtin_popcount(crystal) == 1) {
                    (__builtin_popcount(light ^ crystal) == 1 ? info.crystals_incorrect_primary_extra_1 : info.crystals_incorrect_primary_extra_2) += delta;
                } else {
                    info.crystals_incorrect_secondary_extra += delta;
                }
            }
        }
    }
}

void update_score_info_score(result_info_t & info, cost_t cost, max_t max_) {
    info.score = - 1000000;
    if (info.added_mirrors > max_.mirrors) return;  // You can place at most ??? mirrors.
    if (info.added_obstacles > max_.obstacles) return;  // You can place at most ??? obstacles.
    if (info.lit_lanterns) return;  // A lantern should not be illuminated by any light ray.
    info.score = 0;
    info.score -= info.added_lanterns * cost.lantern;
    info.score -= info.added_mirrors * cost.mirror;
    info.score -= info.added_obstacles * cost.obstacle;
    info.score += info.crystals_primary_ok * 20;
    info.score += info.crystals_secondary_ok * 30;
    info.score -= info.crystals_incorrect * 10;
}

tuple<int, int, int> update_score_shoot_ray(int h, int w, string const & board, int y0, int x0, int dir0, color_t color, result_info_t & info, vector<uint8_t> & light, bool is_positive) {
#ifdef LOCAL
    assert (color == I_BLUE or color == I_YELLOW or color == I_RED);
#endif
    int y = y0;
    int x = x0;
    int dir = dir0;
    while (true) {
        y += neighborhood4_y[dir];
        x += neighborhood4_x[dir];
        if (y < 0 or h <= y or x < 0 or w <= x) {
            return make_tuple(-1, -1, -1);
        }
        info.lit_count -= bool(light[y * w + x] & (0x3 << (2 * dir)));
        light[y * w + x] ^= __builtin_ffs(color) << (2 * dir);
        info.lit_count += bool(light[y * w + x] & (0x3 << (2 * dir)));
        letter_t c = board[y * w + x];
        if (c == C_EMPTY) {
            // nop
        } else if (c == C_MIRROR1 or c == C_MIRROR2) {
            dir = apply_mirror(c, dir);
        } else {
            break;
        }
        if (not is_positive and y == y0 and x == x0) break;
    }
    return make_tuple(y, x, dir);
}

int count_rays(uint8_t l) {
    return __builtin_popcount(((l & 0xaa) >> 1) | (l & 0x55));
}

void update_score_hit_ray(int h, int w, string const & board, int y, int x, int dir, color_t color, result_info_t & info, vector<uint8_t> const & light) {
#ifdef LOCAL
    assert (color == I_BLUE or color == I_YELLOW or color == I_RED);
#endif
    if (dir == -1) return;
    letter_t b = board[y * w + x];
    uint8_t l_prv = light[y * w + x] ^ (__builtin_ffs(color) << (2 * dir));
    uint8_t l_cur = light[y * w + x];
    if (isdigit(b)) {
        update_score_info_crystal(info, b - '0', summarize_light(l_prv), -1);
        update_score_info_crystal(info, b - '0', summarize_light(l_cur), +1);
    } else if (b == C_LANTERN) {
        info.lit_lanterns -= count_rays(l_prv);
        info.lit_lanterns += count_rays(l_cur);
    }
}

color_t get_color_for_dir(uint8_t light, int dir) {
    int ffs_color = (light >> (2 * dir)) & 0x3;
    if (not ffs_color) return 0;
    return 1 << (ffs_color - 1);
}

void update_score_info_add_command(int h, int w, string & board, cost_t cost, max_t max_, result_info_t & info, vector<uint8_t> & light, output_t command);
void update_score_info_remove_command(int h, int w, string & board, cost_t cost, max_t max_, result_info_t & info, vector<uint8_t> & light, output_t command);

void update_score_info_add_command(int h, int w, string & board, cost_t cost, max_t max_, result_info_t & info, vector<uint8_t> & light, output_t command) {
    int y, x; char c; tie(y, x, c) = command;
#ifdef LOCAL
    assert (0 <= y and y < h and 0 <= x and x < w);
    assert (board[y * w + x] == C_EMPTY);
#endif
    auto shoot = [&](int dir, color_t color, bool is_positive) {
        int ny, nx, ndir; tie(ny, nx, ndir) = update_score_shoot_ray(h, w, board, y, x, dir, color, info, light, is_positive);
        update_score_hit_ray(h, w, board, ny, nx, ndir, color, info, light);
    };
    uint8_t preserved_l = light[y * w + x];
    REP (dir, 4) {
        color_t color = get_color_for_dir(preserved_l, dir);
        if (color) shoot(dir, color, false);
    }
    if (c == C_BLUE or c == C_YELLOW or c == C_RED) {
        board[y * w + x] = C_LANTERN;  // NOTE: must be here because they may light themselves
        ++ info.added_lanterns;
        info.lit_lanterns += count_rays(light[y * w + x]);
        REP (dir, 4) {
            shoot(dir, c - '0', true);
        }
    } else if (c == C_MIRROR1 or c == C_MIRROR2) {
	board[y * w + x] = c;
	++ info.added_mirrors;
        uint8_t preserved_l = light[y * w + x];
        REP (dir, 4) {
            color_t color = get_color_for_dir(preserved_l, dir);
            if (color) shoot(apply_mirror(c, dir), color, true);
        }
    } else if (c == C_OBSTACLE) {
	board[y * w + x] = c;
        ++ info.added_obstacles;
    } else {
        assert (false);
    }
    update_score_info_score(info, cost, max_);
}

void update_score_info_remove_command(int h, int w, string & board, cost_t cost, max_t max_, result_info_t & info, vector<uint8_t> & light, output_t command) {
    int y, x; char c; tie(y, x, c) = command;
#ifdef LOCAL
    assert (0 <= y and y < h and 0 <= x and x < w);
    assert (board[y * w + x] == (isdigit(c) ? C_LANTERN : c));
#endif
    auto shoot = [&](int dir, color_t color, bool is_positive) {
        int ny, nx, ndir; tie(ny, nx, ndir) = update_score_shoot_ray(h, w, board, y, x, dir, color, info, light, is_positive);
        update_score_hit_ray(h, w, board, ny, nx, ndir, color, info, light);
    };
    if (c == C_BLUE or c == C_YELLOW or c == C_RED) {
        REP (dir, 4) {
            shoot(dir, c - '0', false);
        }
        info.lit_lanterns -= count_rays(light[y * w + x]);
        board[y * w + x] = C_EMPTY;
        -- info.added_lanterns;
    } else if (c == C_MIRROR1 or c == C_MIRROR2) {
        uint8_t preserved_l = light[y * w + x];
        REP (dir, 4) {
            color_t color = get_color_for_dir(preserved_l, dir);
            if (color) shoot(apply_mirror(c, dir), color, false);
        }
        board[y * w + x] = C_EMPTY;
	-- info.added_mirrors;
    } else if (c == C_OBSTACLE) {
        board[y * w + x] = C_EMPTY;
        -- info.added_obstacles;
    } else {
        assert (false);
    }
    uint8_t preserved_l = light[y * w + x];
    REP (dir, 4) {
        color_t color = get_color_for_dir(preserved_l, dir);
        if (color) shoot(dir, color, true);
    }
    update_score_info_score(info, cost, max_);
}


/******************************************************************************
 * functions for debug
 ******************************************************************************/

bool validate_result_info(int h, int w, string const & original_board, cost_t cost, max_t max_, vector<output_t> const & commands, string const & result_board, vector<uint8_t> const & result_light, result_info_t const & result_info) {
    string board = original_board;
    result_info_t info = {};

    // deploy items to the board
    for (auto command : commands) {
        int y, x; char c; tie(y, x, c) = command;
        if (c == C_BLUE or c == C_YELLOW or c == C_RED) {
            board[y * w + x] = C_LANTERN;
            ++ info.added_lanterns;
        } else if (c == C_MIRROR1 or c == C_MIRROR2) {
            board[y * w + x] = c;
            ++ info.added_mirrors;
        } else if (c == C_OBSTACLE) {
            board[y * w + x] = c;
            ++ info.added_obstacles;
        } else {
            assert (false);
        }
    }

    // chase rays from lanterns
    vector<uint8_t> light(h * w);
    for (auto command : commands) {
        int y0, x0; char c0; tie(y0, x0, c0) = command;
        if (not isdigit(c0)) continue;
        REP (dir0, 4) {
            int y = y0;
            int x = x0;
            int dir = dir0;
            while (true) {
                y += neighborhood4_y[dir];
                x += neighborhood4_x[dir];
                if (y < 0 or h <= y or x < 0 or w <= x) break;
                light[y * w + x] |= (uint8_t)__builtin_ffs(c0 - '0') << (2 * dir);
                char c = board[y * w + x];
                if (c == C_EMPTY) {
                    // nop
                } else if (c == C_MIRROR1 or c == C_MIRROR2) {
                    dir = apply_mirror(c, dir);
                } else {
                    break;
                }
            }
        }
    }

    // count lit crystals
    REP (y, h) REP (x, w) {
        uint8_t l = light[y * w + x];
        if (not l) continue;
        info.lit_count += count_rays(l);
        letter_t c = board[y * w + x];
        if (isdigit(c)) {
            color_t c1 = c - '0';
            color_t l1 = summarize_light(l);
            if (__builtin_popcount(c1) == 1) {
                if (c1 == l1) {
                    ++ info.crystals_primary_ok;
                } else if ((c1 | l1) == l1) {
                    if (__builtin_popcount(c1 ^ l1) == 1) {
                        ++ info.crystals_incorrect_primary_extra_1;
                    } else {
                        ++ info.crystals_incorrect_primary_extra_2;
                    }
                }
            } else {
                if (c1 == l1) {
                    ++ info.crystals_secondary_ok;
                } else if ((l1 | c1) == c1) {
                    ++ info.crystals_incorrect_secondary_half;
                } else if ((c1 | l1) == l1) {
                    ++ info.crystals_incorrect_secondary_extra;
                }
            }
            if (c1 != l1) {
                ++ info.crystals_incorrect;
            }
        } else if (c == C_LANTERN) {
            info.lit_lanterns += count_rays(l);
        }
    }
    update_score_info_score(info, cost, max_);

    // print info and return result
    if (board == result_board and light == result_light and info == result_info) {
        return true;
    } else {
        cerr << "VALIDATION FAILED" << endl;
        cerr << "board: " << (board == result_board ? "OK" : "NG") << endl;
        cerr << "light: " << (light == result_light ? "OK" : "NG") << endl;
        cerr << "info:  " << (info  == result_info  ? "OK" : "NG") << endl;
        REP (y, h) {
            REP (x, w) cerr << board[y * w + x];
            cerr << '|';
            REP (x, w) cerr << result_board[y * w + x];
            cerr << endl;
        }
        REP (x, 2 * w + 1) cerr << '-';
        cerr << endl;
        REP (y, h) {
            REP (x, w) cerr << char(light[y * w + x] ? summarize_light(light[y * w + x]) + '0' : '.');
            cerr << '|';
            REP (x, w) cerr << char(result_light[y * w + x] ? summarize_light(result_light[y * w + x]) + '0' : '.');
            cerr << endl;
        }
        cerr << "details:" << endl;
        REP (y, h) {
            REP (x, w) {
                if (light[y * w + x]) {
                    fprintf(stderr, "%2x", light[y * w + x]);
                } else {
                    cerr << " .";
                }
            }
            cerr << " | ";
            REP (x, w) {
                if (result_light[y * w + x]) {
                    fprintf(stderr, "%2x", result_light[y * w + x]);
                } else {
                    cerr << " .";
                }
            }
            cerr << endl;
        }
        cerr << endl;
        return false;
    }
}


/******************************************************************************
 * the main function
 ******************************************************************************/

vector<output_t> solve(int h, int w, string const & original_original_board, cost_t cost, max_t max_) {
    double clock_begin = rdtsc();
    random_device device;
    xor_shift_128 gen(device());

    // remove unnecessary items
    string original_board = original_original_board;
    original_board = remove_impossible_crystals(h, w, original_board);
    original_board = fill_unused_area(h, w, original_board);

    // prepare lists
    vector<pair<int, int> > initial_crystals = list_cells_with_letters(h, w, original_board, "123456");
    vector<pair<int, int> > initial_secondary_crystals = list_cells_with_letters(h, w, original_board, "356");
    vector<pair<int, int> > initial_empties = list_cells_with_letters(h, w, original_board, ".");

    // result of SA
    vector<output_t> result;
    result_info_t result_info = {};
#ifdef LOCAL
    string result_board = original_board;
#endif

    // state of SA
    vector<output_t> cur;
    string board = original_board;
    vector<uint8_t> light(h * w);
    result_info_t info = {};
    double evaluated;
    vector<int> cur_reverse(h * w, -1);

    // misc values
    int iteration = 0;
    double temperature = 1;
    double sa_clock_begin = rdtsc();
    double sa_clock_end = clock_begin + TLE * 0.95;

    auto swap_to_back = [&](int i) {
        if (i == (int)cur.size() - 1) return;
        int y1, x1; tie(y1, x1, ignore) = cur[i];
        int y2, x2; tie(y2, x2, ignore) = cur.back();
        swap(cur[i], cur.back());
        swap(cur_reverse[y1 * w + x1], cur_reverse[y2 * w + x2]);
    };
    auto add = [&](output_t command) {
        update_score_info_add_command(h, w, board, cost, max_, info, light, command);
    };
    auto remove = [&](output_t command) {
        update_score_info_remove_command(h, w, board, cost, max_, info, light, command);
    };
    auto evaluate = [&](result_info_t const & info) {
        double acc = info.score;
        acc += max(0.0, temperature - 0.1) * EVAL_PARAM_1 * info.crystals_incorrect_primary_extra_1;
        acc += max(0.0, temperature - 0.1) * EVAL_PARAM_2 * info.crystals_incorrect_secondary_half;
        acc += max(0.0, temperature - 0.1) * EVAL_PARAM_3 * info.crystals_incorrect_secondary_extra;
        acc -= max(0.0, temperature - 0.1) * EVAL_PARAM_4 * info.lit_count;
        return acc;
    };
    const double boltzmann = (BOLTZMANN_1 / 100.0) * (1 + (BOLTZMANN_2 / 100.0) * h * w * 0.0001);
    auto try_update = [&]() {
#ifdef LOCAL
#ifdef DEBUG
        assert (validate_result_info(h, w, original_board, cost, max_, cur, board, light, info));
#endif
#endif
        if (result_info.score < info.score) {
            result = cur;
            result_info = info;
#ifdef LOCAL
            result_board = board;
            cerr << "highscore = " << result_info.score << "  (at " << iteration << ", " << temperature << ")" << endl;
            assert (validate_result_info(h, w, original_board, cost, max_, cur, board, light, info));
#endif
        }
        double next_evaluated = evaluate(info);
        int delta = next_evaluated - evaluated;
        return (delta >= 0 or bernoulli_distribution(exp(boltzmann * delta / temperature))(gen));
    };

    for (; ; ++ iteration) {
        if (temperature < 0.1 or iteration % 128 == 0) {
            double t = rdtsc();
            if (t > sa_clock_end) break;
            temperature = 1 - (t - sa_clock_begin) / (sa_clock_end - sa_clock_begin);
        }
        int prob = get_random_lt(NBHD_PROB_1 + NBHD_PROB_2 + NBHD_PROB_3 + NBHD_PROB_4, gen);
        static const array<char, 6> item_table = { C_BLUE, C_YELLOW, C_RED, C_MIRROR1, C_MIRROR2, C_OBSTACLE };
        static const array<char, 3> item_table_light = { C_BLUE, C_YELLOW, C_RED };
        static const array<char, 3> item_table_not_light = { C_MIRROR1, C_MIRROR2, C_OBSTACLE };
        evaluated = evaluate(info);

        if (prob < NBHD_PROB_1) {  // move one
            if (cur.empty()) continue;
            int i = get_random_lt(cur.size(), gen);
            int dir = get_random_lt(4, gen);
            int amount = uniform_int_distribution<int>(1, 2)(gen);
            int y, x; tie(y, x, ignore) = cur[i];
            int ny = x + amount * neighborhood4_y[dir];
            int nx = y + amount * neighborhood4_x[dir];
            if (ny < 0 or h <= ny or nx < 0 or w <= nx) continue;
            if (board[ny * w + nx] != C_EMPTY) continue;
            remove(cur[i]);
            get<0>(cur[i]) = ny;
            get<1>(cur[i]) = nx;
            add(cur[i]);
            if (try_update()) {
                cur_reverse[ y * w +  x] = -1;
                cur_reverse[ny * w + nx] = i;
            } else {
                remove(cur[i]);
                get<0>(cur[i]) = y;
                get<1>(cur[i]) = x;
                add(cur[i]);
            }

        } else if (prob < NBHD_PROB_1 + NBHD_PROB_2) {  // modify one
            if (cur.empty()) continue;
            int i = get_random_lt(cur.size(), gen);
            char c = choose_random(item_table, gen);
            remove(cur[i]);
            swap(get<2>(cur[i]), c);
            add(cur[i]);
            if (not try_update()) {
                remove(cur[i]);
                swap(get<2>(cur[i]), c);
                add(cur[i]);
            }

        } else if (prob < NBHD_PROB_1 + NBHD_PROB_2 + NBHD_PROB_3) {  // remove one
            if (cur.empty()) continue;
            swap_to_back(get_random_lt(cur.size(), gen));
            auto preserved = cur.back();
            remove(cur.back());
            cur.pop_back();
            if (try_update()) {
                int y, x; tie(y, x, ignore) = preserved;
                cur_reverse[y * w + x] = -1;
            } else {
                cur.push_back(preserved);
                add(cur.back());
            }

        } else {  // add one
            int y, x; tie(y, x) = choose_random(initial_empties, gen);
            vector<output_t> preserved;
            if (board[y * w + x] != C_EMPTY) {
                swap_to_back(cur_reverse[y * w + x]);
                preserved.push_back(cur.back());
                remove(cur.back());
                cur.pop_back();
                cur_reverse[y * w + x] = -1;
            }
            assert (board[y * w + x] == C_EMPTY);
            char c = light[y * w + x] ?
                choose_random(item_table, gen) :
                choose_random(item_table_light, gen);
            if (isdigit(c)) {
                REP (dir, 4) {
                    if (get_color_for_dir(light[y * w + x], dir)) {
                        int ny, nx; tie(ny, nx, ignore) = chase_ray_source(h, w, board, y, x, dir, light);
                        swap_to_back(cur_reverse[ny * w + nx]);
                        preserved.push_back(cur.back());
                        remove(cur.back());
                        cur.pop_back();
                        cur_reverse[ny * w + nx] = -1;
                    }
                }
                assert (not light[y * w + x]);
            }
            cur.emplace_back(y, x, c);
            add(cur.back());
            if (try_update()) {
                cur_reverse[y * w + x] = cur.size() - 1;
            } else {
                remove(cur.back());
                cur.pop_back();
                for (auto command : preserved) {
                    int ny, nx; tie(ny, nx, ignore) = command;
                    add(command);
                    cur.push_back(command);
                    cur_reverse[ny * w + nx] = cur.size() - 1;
                }
            }
        }
    }

#ifdef LOCAL
    REP (y, h) {
        REP (x, w) {
            cerr << result_board[y * w + x];
        }
        cerr<< endl;
    }
#endif
    ll seed = -1;
#ifdef LOCAL
    if (getenv("SEED")) seed = atoll(getenv("SEED"));
    setenv("SCORE", to_string(result_info.score).c_str(), true);
#endif
    original_board = original_original_board;
    int num_empty = count(ALL(original_board), C_EMPTY);
    int num_obstacles = count(ALL(original_board), C_OBSTACLE);
    int num_crystals_primary   = count(ALL(original_board), C_BLUE)  + count(ALL(original_board), C_YELLOW) + count(ALL(original_board), C_RED);
    int num_crystals_secondary = count(ALL(original_board), C_GREEN) + count(ALL(original_board), C_VIOLET) + count(ALL(original_board), C_ORANGE);
    int num_crystals = num_crystals_primary + num_crystals_secondary;
    assert (num_empty + num_obstacles + num_crystals == h * w);
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
    cerr << ",\"numCrystalsPrimary\":" << num_crystals_primary;
    cerr << ",\"numCrystalsSecondary\":" << num_crystals_secondary;
    cerr << ",\"addedLanterns\":" << result_info.added_lanterns;
    cerr << ",\"addedMirrors\":" << result_info.added_mirrors;
    cerr << ",\"addedObstacles\":" << result_info.added_obstacles;
    cerr << ",\"primaryOk\":" << result_info.crystals_primary_ok;
    cerr << ",\"secondaryOk\":" << result_info.crystals_secondary_ok;
    cerr << ",\"incorrect\":" << result_info.crystals_incorrect;
    cerr << ",\"rawScore\":" << result_info.score;
    cerr << ",\"iteration\":" << iteration;
    cerr << ",\"elapsed\":" << elapsed;
    cerr << "}" << endl;
    cerr.flush();
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
