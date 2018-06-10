# Python Version: 3.x
import concurrent.futures
import glob
import json
import multiprocessing
import os
import shutil
import subprocess
import sys
import tempfile
import uuid

class Executer(object):
    def __init__(self, tempdir, sources, seeds):
        self.tempdir = os.path.abspath(tempdir)
        self.sources = []
        for source in sources.split(','):
            preserved = self.tempdir + '/' + os.path.basename(source)
            shutil.copyfile(source, preserved)
            self.sources += [ preserved ]
        self.seeds = seeds

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
                    data = json.loads(line.decode())
                    maximal_score = (20 - data['costLantern'] / 4) * data['numCrystalsPrimary'] + (30 - data['costLantern'] / 2) * data['numCrystalsSecondary']
                    normalized_score = data['rawScore'] / maximal_score
                    print('\tseed = %s:\traw score = %d,\tnormalized score = %f' % (seed, data['rawScore'], normalized_score), file=sys.stderr)
                    sys.stderr.flush()
                    return normalized_score
            else:
                assert False
        except Exception as e:
            print(e, file=sys.stderr)
            print('seed = {}'.format(seed), file=sys.stderr)
            print(repr(result.stdout), file=sys.stderr)
            sys.stderr.flush()
            return 0.0

    def evaluate(self, raw_scores):
        return sum(raw_scores) / len(raw_scores)

    def __call__(self, **kwargs):
        sys.stdout.flush()  # required
        binary = self.compile(kwargs)
        with concurrent.futures.ThreadPoolExecutor(max_workers=multiprocessing.cpu_count()) as executor:
            futures = []
            for seed in self.seeds:
                futures.append(executor.submit(self.execute, binary, seed))
            return self.evaluate([ f.result() for f in futures ])

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
    parser.add_argument('--acq', choices=[ 'ucb', 'ei', 'poi' ], default='ucb')
    parser.add_argument('--plot')
    args = parser.parse_args()

    # optimize
    from bayes_opt import BayesianOptimization  # https://pypi.org/project/bayesian-optimization/
    param_bounds = {
        'BOLTZMANN': (0.0, 2.0),
        'EV1': (-1.0, 2.0),
        'EV2': (-1.0, 2.0),
        'EV3': (-1.0, 2.0),
        'EV4': (-1.0, 2.0),
    }
    kwargs = {
        'init_points': args.init_points,
        'n_iter': args.n_iter,
        'acq': args.acq,
    }
    with tempfile.TemporaryDirectory() as tempdir:
        execute = Executer(tempdir=tempdir, sources=args.sources, seeds=args.seeds)
        optimizer = BayesianOptimization(f=execute, pbounds=param_bounds, verbose=True)
        optimizer.maximize(**kwargs)

    # output
    print(json.dumps(optimizer.res, indent=4))
    if args.plot:
        plot(optimizer, 'BOLTZMANN', args.plot)

if __name__ == '__main__':
    main()
