# Python Version: 3.x
import concurrent.futures
import datetime
import glob
import json
import math
import multiprocessing
import os
import shutil
import subprocess
import sys
import tempfile
import uuid

class Executer(object):
    def __init__(self, tempdir, sources, seeds, log_fh):
        self.tempdir = os.path.abspath(tempdir)
        self.sources = []
        for source in sources.split(','):
            preserved = self.tempdir + '/' + os.path.basename(source)
            shutil.copyfile(source, preserved)
            self.sources += [ preserved ]
        self.seeds = seeds
        self.log_fh = log_fh
        self.max_value = - math.inf

    def compile(self, params):
        defines = ' '.join('-D{}=({})'.format(k, v) for k, v in params.items())
        binary = self.tempdir + '/' + str(uuid.uuid4())
        command = ' '.join([
            os.environ.get('CXX', 'g++'),
            os.environ.get('CXXFLAGS', '-std=c++14 -O2 -DLOCAL'),
            defines,
            '-o', binary,
            self.sources[0],
        ]).split(' ')
        result = subprocess.run(command, stderr=sys.stderr)
        result.check_returncode()
        return binary

    def execute(self, binary, seed):
        try:
            command = [ 'java', '-jar', 'tester.jar', '-exec', binary, '-debug', '-seed', seed, '-novis', '-timeout', str(20) ]
            result = subprocess.run(command, stdout=subprocess.PIPE, stderr=sys.stderr)
            result.check_returncode()
            for line in result.stdout.splitlines():
                if line.startswith(b'{"seed":'):
                    row = json.loads(line.decode())
                    maximal_score = (20 - row['costLantern'] / 4) * row['numCrystalsPrimary'] + (30 - row['costLantern'] / 2) * row['numCrystalsSecondary']
                    normalized_score = row['rawScore'] / maximal_score
                    data = {
                        'seed': seed,
                        'timestamp': str(datetime.datetime.now(datetime.timezone.utc)),
                        'log': result.stdout.decode(),
                        'data': row,
                    }
                    print(json.dumps(data), file=self.log_fh)
                    self.log_fh.flush()
                    return normalized_score
            else:
                assert False
        except Exception as e:
            data = {
                'seed': seed,
                'timestamp': str(datetime.datetime.now(datetime.timezone.utc)),
                'log': result.stdout.decode(),
                'error': str(e),
            }
            print(json.dumps(data), file=self.log_fh)
            self.log_fh.flush()
            return None

    def evaluate(self, raw_scores):
        raw_scores = list(filter(lambda x: x is not None, raw_scores))  # ignore failured seeds
        if not raw_scores:
            return 0.0
        return sum(raw_scores) / len(raw_scores)

    def __call__(self, **kwargs):
        sys.stdout.flush()  # required
        binary = self.compile(kwargs)
        cpu_count = multiprocessing.cpu_count()
        probe = min(len(self.seeds), math.ceil((len(self.seeds) // 10 + 1) / cpu_count) * cpu_count)  # use a multiple of cpu_count
        with concurrent.futures.ThreadPoolExecutor(max_workers=cpu_count) as executor:
            futures = []
            for seed in self.seeds[: probe]:
                futures.append(executor.submit(self.execute, binary, seed))
            value = self.evaluate([ f.result() for f in futures ])
            if value > self.max_value / 2:  # check whether the params should be examined exactly
                for seed in self.seeds[probe :]:
                    futures.append(executor.submit(self.execute, binary, seed))
                value = self.evaluate([ f.result() for f in futures ])
        print(json.dumps({ 'params': kwargs, 'value': value }), file=self.log_fh)
        self.log_fh.flush()
        self.max_value = max(self.max_value, value)
        return value

def plot(optimizer, name, path):
    import matplotlib.pyplot as plt
    import numpy as np
    import seaborn as sns
    xs = np.array([ params[name] for params in optimizer.res['all']['params'] ])
    ys = np.array(optimizer.res['all']['values'])
    plt.figure(figsize=(16, 9))
    sns.regplot(xs, ys, order=4)  # order=2 is weak, order=3 is 
    plt.savefig(path)

def main():
    # input
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('seeds', nargs='+')
    parser.add_argument('--sources', default='main.cpp,CrystalLighting.cpp')
    parser.add_argument('--n-iter', type=int, default=90)
    parser.add_argument('--init-points', type=int, default=10)
    # parser.add_argument('--acq', choices=[ 'ucb', 'ei', 'poi' ], default='ucb')
    parser.add_argument('--log-file', default='/dev/null')
    parser.add_argument('--plot')
    args = parser.parse_args()

    # optimize
    from bayes_opt import BayesianOptimization  # https://pypi.org/project/bayesian-optimization/
    param_bounds = {
        'BOLTZMANN_1': (0, 200),
        'BOLTZMANN_2': (0, 100),
        'NBHD_PROB_1': (0, 100),
        'NBHD_PROB_2': (0, 30),
        'NBHD_PROB_3': (0, 30),
        'NBHD_PROB_4': (0, 100),
    }
    kwargs = {
        'init_points': args.init_points,
        'n_iter': args.n_iter,
        # 'acq': args.acq,
        'acq': 'ucb',
        'kappa': 1.0,
    }
    with open(args.log_file, 'w') as log_fh:
        with tempfile.TemporaryDirectory() as tempdir:
            execute = Executer(tempdir=tempdir, sources=args.sources, seeds=args.seeds, log_fh=log_fh)
            optimizer = BayesianOptimization(f=execute, pbounds=param_bounds, verbose=True)
            optimizer.maximize(**kwargs)
        print(json.dumps(optimizer.res), file=log_fh)

    # output
    print(json.dumps(optimizer.res, indent=4))
    if args.plot:
        plot(optimizer, 'BOLTZMANN', args.plot)

if __name__ == '__main__':
    main()
