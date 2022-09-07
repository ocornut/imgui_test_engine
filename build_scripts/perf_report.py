#!/usr/bin/env python
"""
perf_report.py
==============

This is a helper script aimed for simplifying testing performance of code changes using perf test suite.
Script does following:
1. Automatically checks out required branch
  - git executable is required on system PATH (Posix/Windows)
2. Builds test suite project
  - make used on Posix systems
  - msbuild used on Windows systems
3. Runs perf tests
  - A subset of perf tests may be run by specifying filter mask with -r / --run parameters
  - Test results of each run are appended to imgui_perflog.csv
4. Starts test suite for result preview
  - Open perf tool: "Tools > Perf Tool"

Script has three main modes of operation:
- Data cleanup (--clean)
- Test execution (--run)
- Report generation (--output)
All these commands can be freely mixed together.


### Examples

Generate remove old data (--clean), build and run drawing-related perf tests (--run perf_draw) on master and
feature/branch (--branches master feature/branch) in order to evaluate performance impact of changes in feature/branch.
Finally, show generated html report in a browser (--show).
  perf_report.py --clean --branches master feature/branch --run perf_draw --show

Run all (lack of -r / --run) perf tests again (possibly on a different OS) and append to existing imgui_perflog.csv
file (--force). Generated html report will also be overwritten (--force).
  perf_report.py --force --branches master feature/branch

Generate a new html report by using existing imgui_perflog.csv data and save it to specified file (--output).
  perf_report.py --output perf_draw_report.html

For more command line parameters and their descriptions see command line help.
  perf_report.py --help


### Command line parameters

usage: perf_report.py [-h] [-c] [-f] [-b [BRANCHES ...]] [-x STRESS] [-r [MASK]] [-p] [-o [OUTPUT]] [-s]

options:
  -h, --help            show this help message and exit
  -c, --clean           Clear previous results (imgui_perflog.csv).
  -f, --force           Overwrite report file if exists.
  -b [BRANCHES ...], --branches [BRANCHES ...]
                        Git branches to perform runs on. Branches must already exist. Not specifying this parameter uses current branch.
  -x STRESS, --stress STRESS
                        Perf stress amount. Used with -r.
  -r [MASK], --run [MASK]
                        Run tests matching specified mask (eg. perf_draw,perf_misc). Not specifying a mask will run all perf tests.
  -p, --preview         Preview results in imgui_test_suite after executing tests of each branch.
  -o [OUTPUT], --output [OUTPUT]
                        Report file name. Default: capture_perf_report.html
  -s, --show            Open generated report in the browser.
"""
import argparse
import logging
import multiprocessing
import os
import subprocess
import sys
import webbrowser


def get_git_branch():
    with open('../../imgui/.git/HEAD') as fp:
        branch = fp.read().strip()
        if branch.startswith('ref: refs/heads/'):
            return branch[16:]


def git_checkout(branch):
    try:
        if subprocess.call(['git', 'checkout', branch], cwd='../../imgui') != 0:
            return -1
    except subprocess.CalledProcessError:
        logging.error('Checking out branch {} failed.'.format(branch))
        return -1


def main():
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--clean', action='store_true', help='Clear previous results (imgui_perflog.csv).')
    parser.add_argument('-f', '--force', action='store_true', help='Overwrite report file if exists.')
    parser.add_argument('-b', '--branches', nargs='*', required=False, help='Git branches to perform runs on. Branches '
                                                                            'must already exist. Not specifying this '
                                                                            'parameter uses current branch.')
    parser.add_argument('-x', '--stress', default=5, type=int, help='Perf stress amount. Used with -r.')
    parser.add_argument('-r', '--run', nargs='?', const='perf', required=False, metavar='MASK',
                        help='Run tests matching specified mask (eg. perf_draw,perf_misc). Not specifying a mask will '
                             'run all perf tests.')
    parser.add_argument('-p', '--preview', action='store_true', help='Preview results in imgui_test_suite after executing '
                                                                     'tests of each branch.')
    parser.add_argument('-o', '--output', nargs='?', const='capture_perf_report.html',
                        help='Report file name. Default: capture_perf_report.html')
    parser.add_argument('-s', '--show', action='store_true', help='Open generated report in the browser.')
    args = parser.parse_args()

    if os.name == 'nt':
        imgui_test_suite_exe = '../imgui_test_suite/Release/imgui_test_suite.exe'
    else:
        imgui_test_suite_exe = '../imgui_test_suite/imgui_test_suite'
    initial_branch = get_git_branch()

    if args.clean:
        for file in (args.output, 'imgui_perflog.csv'):
            if file is not None and os.path.isfile(file):
                os.unlink(file)
                logging.info('Removed '.format(file))

    if args.run:
        if not args.force:
            if os.path.isfile('imgui_perflog.csv'):
                logging.error('File imgui_perflog.csv already exists. Either move, use -f or -c.')
                return -1

        for branch in args.branches or [None]:
            if branch:
                git_checkout(branch)
            else:
                branch = get_git_branch()

            try:
                if os.name == 'nt':
                    msbuild = os.environ['WINDIR'] + '/Microsoft.NET/Framework/v4.0.30319/MSBuild.exe'
                    subprocess.call([msbuild, '../imgui_test_suite/imgui_test_suite.vcxproj', '/t:Clean',
                                     '/p:Configuration=Release'])
                    subprocess.call([msbuild, '../imgui_test_suite/imgui_test_suite.vcxproj', '/p:Configuration=Release',
                                     '/m:' + str(multiprocessing.cpu_count())])
                else:
                    subprocess.call(['make', '-C', '../imgui_test_suite', 'clean'])
                    subprocess.call(['make', '-C', '../imgui_test_suite', '-j'+str(multiprocessing.cpu_count())])
            except subprocess.CalledProcessError:
                logging.error('Building imgui_test_suite on branch {} failed'.format(branch))
                return -1

            try:
                subprocess.call([imgui_test_suite_exe, '-nogui', '-nopause', '-v2', '-ve4', '-stressamount',
                                 str(args.stress), args.run])
            except subprocess.CalledProcessError:
                logging.error('imgui_test_suite returned an error when executing tests.')
                return -1

            if args.preview:
                logging.info('Opening imgui_test_suite for result preview. Close the window to continue.')
                try:
                    # FIXME: This could open perf tool from the start.
                    subprocess.call([imgui_test_suite_exe, '-gui', '-v2', '-ve4', '-stressamount', str(args.stress)])
                except subprocess.CalledProcessError:
                    logging.error('imgui_test_suite returned an error when previewing results.')
                    return -1

    if args.output:
        if not args.force:
            if os.path.isfile(args.output):
                logging.error('File {} already exists. Either move it, use -f or -c')
                return -1
        try:
            env = dict(os.environ)
            env['CAPTURE_PERF_REPORT_OUTPUT'] = args.output
            subprocess.call([imgui_test_suite_exe, '-gui', '-nopause', '-v2', '-ve4', 'capture_perf_report'], env=env)
        except subprocess.CalledProcessError:
            logging.error('imgui_test_suite returned an error when generating a performance report.')
            return -1
        else:
            if args.show:
                webbrowser.open(os.path.abspath(args.output), new=2)

    if len(sys.argv) == 1:
        parser.print_help()
    else:
        git_checkout(initial_branch)


if __name__ == '__main__':
    try:
        result = main()
    except KeyboardInterrupt:
        result = 0
    except Exception as e:
        result = -1
        logging.exception(e)
    sys.exit(result)
