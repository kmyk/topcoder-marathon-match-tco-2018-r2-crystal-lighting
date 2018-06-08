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

    if args.what == 'table':
        headers = [ df.index.name ] + list(df.columns)
        for key in list(headers):
            if key.endswith('samples'):
                df = df.drop(key, axis=1)
                headers.remove(key)
        df = df.sort_index()
        s = tabulate(df, headers=headers, showindex='always', tablefmt='orgtbl')
        lines = s.splitlines()
        lines[1] = lines[1].replace('+', '|')
        s = '\n'.join(lines)
        print(s)

    elif args.what == 'summary':
        print('average of raw score =', df['raw_score'].mean())

    elif args.what == 'compare':
        if args.compare is None:
            parser.error('the following arguments are required: --compare')
        df1 = df
        df2 = load_list_of_json_file(args.compare)
        df1 = df1.rename(columns={ 'raw_score': 'raw_score_1' })
        df2 = df2.rename(columns={ 'raw_score': 'raw_score_2' })
        df = df1.join(df2[ [ 'raw_score_2' ] ])
        df = df.assign(raw_score_diff=lambda row: row.raw_score_1 - row.raw_score_2)
        df = df.sort_values(by='raw_score_diff')
        headers = list(df.columns)
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
