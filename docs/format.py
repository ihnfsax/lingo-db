from subprocess import Popen, PIPE
import argparse
import os
import sys
import re

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Format C++ function calls in a mlir file")
    parser.add_argument("file", help="The file to format")
    args = parser.parse_args()

    if not os.path.isfile(args.file):
        print(f'File "{args.file}" does not exist')
        sys.exit(1)

    with open(args.file, "r") as f:
        content = f.read()
        result = re.findall(r'@(\w+)[^a-zA-Z0-9_]', content)
        result += re.findall(r'\"(\w+)\"', content)
        result = list(set(result))
        for func_call in result:
            process = Popen(['c++filt', func_call], stdout=PIPE, stderr=PIPE)
            stdout, stderr = process.communicate()
            stdout = stdout.decode("utf-8").strip()
            if stdout != func_call:
                content = content.replace("@" + func_call, '@"' + stdout + '"')
                content = content.replace("\"" + func_call, '"' + stdout)

    with open(args.file, "w") as f:
        f.write(content)
