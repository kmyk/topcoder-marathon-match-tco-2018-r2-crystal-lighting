# Python Version: 3.x
import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import collections
import json
from tabulate import tabulate  # https://pypi.org/project/tabulate/

def load_list_of_json_file(path):
    df = []
    with open(path) as fh:
        decoder = json.JSONDecoder(object_pairs_hook=collections.OrderedDict)
        for i, line in enumerate(fh):
            df += [ decoder.decode(line) ]
    df = pd.DataFrame(df, columns=df[0].keys())
    df.set_index('seed', inplace=True)
    return df

def main():
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('what', choices='table summary compare pairplot distplot')
    parser.add_argument('file', nargs='?', default='/dev/stdin')
    parser.add_argument('--seed', type=int)
    parser.add_argument('--compare')
    parser.add_argument('--save')
    args = parser.parse_args()

    df = load_list_of_json_file(args.file)
    df = df.assign(maximalScore=lambda row: (20 - row.costLantern / 4) * row.numCrystalsPrimary + (30 - row.costLantern / 2) * row.numCrystalsSecondary)
    df = df.assign(normalizedScore=lambda row: row.rawScore / row.maximalScore)

    if args.what == 'table':
        headers = 'seed H W costLantern costMirror costObstacle maxMirrors maxObstacles addedLanterns addedMirrors addedObstacles primaryOk secondaryOk incorrect iteration elapsed rawScore maximalScore normalizedScore'.split()
        for key in list(df.columns):
            if key not in headers:
                df = df.drop(key, axis=1)
        df = df.sort_index()
        s = tabulate(df, headers=headers, showindex='always', tablefmt='orgtbl')
        lines = s.splitlines()
        lines[1] = lines[1].replace('+', '|')
        s = '\n'.join(lines)
        print(s)

    elif args.what == 'summary':
        print('average of        raw score =', df['rawScore'].mean())
        print('average of normalized score =', df['normalizedScore'].mean())
        print('max     of normalized score =', df['normalizedScore'].max())
        print('median  of normalized score =', df['normalizedScore'].median())
        print('min     of normalized score =', df['normalizedScore'].min())

    elif args.what == 'compare':
        if args.compare is None:
            parser.error('the following arguments are required: --compare')
        df1 = df
        df2 = load_list_of_json_file(args.compare)
        df1 = df1.rename(columns={ 'rawScore': 'rawScore1' })
        df2 = df2.rename(columns={ 'rawScore': 'rawScore2' })
        df = df1.join(df2[ [ 'rawScore2' ] ])
        df = df.assign(rawScoreDiff=lambda row: row.rawScore1 - row.rawScore2)
        df = df.sort_values(by='rawScoreDiff')
        headers = 'seed H W costLantern costMirror costObstacle maxMirrors maxObstacles rawScore1 rawScore2 rawScoreDiff'.split()
        for key in list(df.columns):
            if key not in headers:
                df = df.drop(key, axis=1)
        s = tabulate(df, headers=headers, showindex='always', tablefmt='orgtbl')
        lines = s.splitlines()
        lines[1] = lines[1].replace('+', '|')
        s = '\n'.join(lines)
        print(s)

    else:
        assert False


if __name__ == '__main__':
    main()
