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

vector<pair<int, int> > list_ray_sources(int h, int w, string const & board, int y0, int x0, vector<uint8_t> const & light) {
    vector<pair<int, int> > results;
    REP (dir0, 4) {
        if (not (light[y0 * w + x0] & (0x3 << (2 * dir0)))) continue;
        int dir = dir0;
        int y = y0;
        int x = x0;
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
        if (y == y0 and x == x0) continue;
        if (find(ALL(results), make_pair(y, x)) != results.end()) continue;
        results.emplace_back(y, x);
    }
    return results;
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

tuple<int, int, int> update_score_shoot_ray(int h, int w, string const & board, int y, int x, int dir, color_t color, result_info_t & info, vector<uint8_t> & light) {
    static int used[MAX_H * MAX_W];
    static int clock;
    ++ clock;
#ifdef LOCAL
    assert (color == I_BLUE or color == I_YELLOW or color == I_RED);
#endif
    while (true) {
        y += neighborhood4_y[dir];
        x += neighborhood4_x[dir];
        if (y < 0 or h <= y or x < 0 or w <= x) {
            y = x = dir = -1;
            break;
        }
        if (used[y * w + x] != clock) {  // ignore if loop exists
            info.lit_count -= bool(light[y * w + x] & (0x3 << (2 * dir)));
            light[y * w + x] ^= __builtin_ffs(color) << (2 * dir);
            info.lit_count += bool(light[y * w + x] & (0x3 << (2 * dir)));
        }
        used[y * w + x] = clock;
        letter_t c = board[y * w + x];
        if (c == C_EMPTY) {
            // nop
        } else if (c == C_MIRROR1 or c == C_MIRROR2) {
            dir = apply_mirror(c, dir);
        } else {
            break;
        }
    }
    return make_tuple(y, x, dir);
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
        info.lit_lanterns -= int(bool(l_prv & 0x03)) + int(bool(l_prv & 0x0c)) + int(bool(l_prv & 0x30)) + int(bool(l_prv & 0xc0));
        info.lit_lanterns += int(bool(l_cur & 0x03)) + int(bool(l_cur & 0x0c)) + int(bool(l_cur & 0x30)) + int(bool(l_cur & 0xc0));
    }
}

color_t get_color_for_dir(uint8_t light, int dir) {
    int ffs_color = (light >> (2 * dir)) & 0x3;
    if (not ffs_color) return 0;
    return 1 << (ffs_color - 1);
}

void update_score_info_add(int h, int w, string & board, cost_t cost, max_t max_, result_info_t & info, vector<uint8_t> & light, output_t command);
void update_score_info_remove(int h, int w, string & board, cost_t cost, max_t max_, result_info_t & info, vector<uint8_t> & light, output_t command);

void update_score_info_add(int h, int w, string & board, cost_t cost, max_t max_, result_info_t & info, vector<uint8_t> & light, output_t command) {
    int y, x; char c; tie(y, x, c) = command;
#ifdef LOCAL
    assert (0 <= y and y < h and 0 <= x and x < w);
    assert (board[y * w + x] == C_EMPTY);
#endif
    REP (dir, 4) {
        color_t color = get_color_for_dir(light[y * w + x], dir);
        if (not color) continue;
        int ny, nx, ndir; tie(ny, nx, ndir) = update_score_shoot_ray(h, w, board, y, x, dir, color, info, light);
        update_score_hit_ray(h, w, board, ny, nx, ndir, color, info, light);
    }
    if (c == C_BLUE or c == C_YELLOW or c == C_RED) {
        board[y * w + x] = C_LANTERN;  // NOTE: must be here because they may light themselves
        REP (dir, 4) {
            int ny, nx, ndir; tie(ny, nx, ndir) = update_score_shoot_ray(h, w, board, y, x, dir, c - '0', info, light);
            update_score_hit_ray(h, w, board, ny, nx, ndir, c - '0', info, light);
        }
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
    if (c == C_MIRROR1 or c == C_MIRROR2) {
        REP (dir, 4) {
            color_t color = get_color_for_dir(light[y * w + x], dir);
            if (not color) continue;
            int ny, nx, ndir; tie(ny, nx, ndir) = update_score_shoot_ray(h, w, board, y, x, apply_mirror(c, dir), color, info, light);
            update_score_hit_ray(h, w, board, ny, nx, ndir, color, info, light);
        }
    }
    update_score_info_score(info, cost, max_);
}

void update_score_info_remove(int h, int w, string & board, cost_t cost, max_t max_, result_info_t & info, vector<uint8_t> & light, output_t command) {
    int y, x; char c; tie(y, x, c) = command;
#ifdef LOCAL
    assert (0 <= y and y < h and 0 <= x and x < w);
    assert (board[y * w + x] == (isdigit(c) ? C_LANTERN : c));
#endif
    if (c == C_MIRROR1 or c == C_MIRROR2) {
        REP (dir, 4) {
            color_t color = get_color_for_dir(light[y * w + x], dir);
            if (not color) continue;
            int ny, nx, ndir; tie(ny, nx, ndir) = update_score_shoot_ray(h, w, board, y, x, apply_mirror(c, dir), color, info, light);
            update_score_hit_ray(h, w, board, ny, nx, ndir, color, info, light);
        }
    }
    if (c == C_BLUE or c == C_YELLOW or c == C_RED) {
        REP (dir, 4) {
            int ny, nx, ndir; tie(ny, nx, ndir) = update_score_shoot_ray(h, w, board, y, x, dir, c - '0', info, light);
            update_score_hit_ray(h, w, board, ny, nx, ndir, c - '0', info, light);
        }
        -- info.added_lanterns;
    } else if (c == C_MIRROR1 or c == C_MIRROR2) {
	-- info.added_mirrors;
    } else if (c == C_OBSTACLE) {
        -- info.added_obstacles;
    } else {
        assert (false);
    }
    board[y * w + x] = C_EMPTY;
    REP (dir, 4) {
        color_t color = get_color_for_dir(light[y * w + x], dir);
        if (not color) continue;
        int ny, nx, ndir; tie(ny, nx, ndir) = update_score_shoot_ray(h, w, board, y, x, dir, color, info, light);
        update_score_hit_ray(h, w, board, ny, nx, ndir, color, info, light);
    }
    update_score_info_score(info, cost, max_);
}

int compute_score(int h, int w, string board, cost_t cost, max_t max_, vector<output_t> const & commands) {
    vector<uint8_t> light(h * w);
    result_info_t info = {};
    for (auto cmd : commands) {
        update_score_info_add(h, w, board, cost, max_, info, light, cmd);
    }
    return info.score;
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
    result_info_t result_info = {};

    // state of SA
    vector<output_t> cur;
    vector<uint8_t> light(h * w);
    result_info_t info = {};
    double evaluated;

    // misc values
    int iteration = 0;
    double temperature = 1;
    double sa_clock_begin = rdtsc();
    double sa_clock_end = clock_begin + TLE * 0.95;

    auto add = [&](output_t command) {
        update_score_info_add(h, w, board, cost, max_, info, light, command);
    };
    auto remove = [&](output_t command) {
        update_score_info_remove(h, w, board, cost, max_, info, light, command);
    };
    auto evaluate = [&](result_info_t const & info) {
        double acc = info.score;
        acc += max(0.0, temperature - 0.1) * info.crystals_incorrect_primary_extra_1 * 5;
        acc += max(0.0, temperature - 0.1) * info.crystals_incorrect_secondary_half * 40;
        acc += max(0.0, temperature - 0.1) * info.crystals_incorrect_secondary_extra * 10;
        acc -= max(0.0, temperature - 0.1) * info.lit_count * 0.1;
        return acc;
    };
    auto try_update = [&]() {
        if (result_info.score < info.score) {
            result = cur;
            result_info = info;
#ifdef LOCAL
            cerr << "highscore = " << result_info.score << "  (at " << iteration << ", " << temperature << ")" << endl;
#endif
        }
        double next_evaluated = evaluate(info);
        int delta = next_evaluated - evaluated;
        constexpr double boltzmann = 0.2;
        return (delta >= 0 or bernoulli_distribution(exp(boltzmann * delta / temperature))(gen));
    };

    for (; ; ++ iteration) {
        if (temperature < 0.1 or iteration % 128 == 0) {
            double t = rdtsc();
            if (t > sa_clock_end) break;
            temperature = 1 - (t - sa_clock_begin) / (sa_clock_end - sa_clock_begin);
        }
        int prob = uniform_int_distribution<int>(0, 99)(gen);
        static const array<char, 6> item_table = { C_BLUE, C_YELLOW, C_RED, C_MIRROR1, C_MIRROR2, C_OBSTACLE };
        static const array<char, 3> item_table_light = { C_BLUE, C_YELLOW, C_RED };
        static const array<char, 3> item_table_not_light = { C_MIRROR1, C_MIRROR2, C_OBSTACLE };
        evaluated = evaluate(info);

        if (prob < 50) {  // move one
            if (cur.empty()) continue;
            int i = get_random_lt(cur.size(), gen);
            int dir = get_random_lt(4, gen);
            int amount = uniform_int_distribution<int>(1, 2)(gen);
            {
                int ny = get<0>(cur[i]) + amount * neighborhood4_y[dir];
                int nx = get<1>(cur[i]) + amount * neighborhood4_x[dir];
                if (ny < 0 or h <= ny or nx < 0 or w <= nx) continue;
                if (board[ny * w + nx] != C_EMPTY) continue;
            }
            remove(cur[i]);
            get<0>(cur[i]) += amount * neighborhood4_y[dir];
            get<1>(cur[i]) += amount * neighborhood4_x[dir];
            add(cur[i]);
            if (not try_update()) {
                remove(cur[i]);
                get<0>(cur[i]) -= amount * neighborhood4_y[dir];
                get<1>(cur[i]) -= amount * neighborhood4_x[dir];
                add(cur[i]);
            }

        } else if (prob < 60) {  // modify one
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

        } else if (prob < 80) {  // remove one
            if (cur.empty()) continue;
            int i = get_random_lt(cur.size(), gen);
            swap(cur[i], cur.back());
            auto preserved = cur.back();
            remove(cur.back());
            cur.pop_back();
            if (not try_update()) {
                cur.push_back(preserved);
                add(cur.back());
            }

        } else {  // add one
            int y = get_random_lt(h, gen);
            int x = get_random_lt(w, gen);
            if (board[y * w + x] != C_EMPTY) continue;
            char c = light[y * w + x] ? 
                choose_random(item_table_not_light, gen) :
                choose_random(item_table_light, gen);
            cur.emplace_back(y, x, c);
            add(cur.back());
            if (not try_update()) {
                remove(cur.back());
                cur.pop_back();
            }
        }
    }

    ll seed = -1;
#ifdef LOCAL
    if (getenv("SEED")) seed = atoll(getenv("SEED"));
    setenv("SCORE", to_string(result_info.score).c_str(), true);
#endif
    int num_empty = count(ALL(board), C_EMPTY);
    int num_obstacles = count(ALL(board), C_OBSTACLE);
    int num_crystals = h * w - num_obstacles - num_empty;
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