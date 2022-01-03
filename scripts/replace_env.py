#!/bin/env python3
import os

# ROOT_DIR = os.path.dirname(os.path.abspath(__file__))

Import("env")

# access to global construction environment
# print(env)

# # Dump construction environment (for debug purpose)
# print(env.Dump())


def getEnvContent(root):
    with open(os.path.abspath(os.path.join(root, './.env'))) as file:
        return file.read()


def main():
    content = getEnvContent('./')
    for line in content.split('\n'):
        if not line:
            continue
        
        variable, value = line.split('=')
        
        if variable.startswith('#'):
            continue

        if variable.startswith('--'):
            env.Append(UPLOAD_FLAGS=[line])
        else:
            env.Append(BUILD_FLAGS=[f'-D{line}'])


main()
