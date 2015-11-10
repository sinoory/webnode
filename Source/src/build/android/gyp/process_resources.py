#!/usr/bin/env python
#
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Process Android resources to generate R.java, and prepare for packaging.

This will crunch images and generate v14 compatible resources
(see generate_v14_compatible_resources.py).
"""

import optparse
import os
import re
import shutil
import sys
import zipfile

import generate_v14_compatible_resources

from util import build_utils

def ParseArgs(args):
  """Parses command line options.

  Returns:
    An options object as from optparse.OptionsParser.parse_args()
  """
  parser = optparse.OptionParser()
  build_utils.AddDepfileOption(parser)

  parser.add_option('--android-sdk', help='path to the Android SDK folder')
  parser.add_option('--android-sdk-tools',
                    help='path to the Android SDK build tools folder')
  parser.add_option('--non-constant-id', action='store_true')

  parser.add_option('--android-manifest', help='AndroidManifest.xml path')
  parser.add_option('--custom-package', help='Java package for R.java')

  parser.add_option('--resource-dirs',
                    help='Directories containing resources of this target.')
  parser.add_option('--dependencies-res-zips',
                    help='Resources from dependents.')

  parser.add_option('--resource-zip-out',
                    help='Path for output zipped resources.')

  parser.add_option('--R-dir',
                    help='directory to hold generated R.java.')
  parser.add_option('--srcjar-out',
                    help='Path to srcjar to contain generated R.java.')

  parser.add_option('--proguard-file',
                    help='Path to proguard.txt generated file')

  parser.add_option(
      '--v14-verify-only',
      action='store_true',
      help='Do not generate v14 resources. Instead, just verify that the '
      'resources are already compatible with v14, i.e. they don\'t use '
      'attributes that cause crashes on certain devices.')

  parser.add_option(
      '--extra-res-packages',
      help='Additional package names to generate R.java files for')
  parser.add_option(
      '--extra-r-text-files',
      help='For each additional package, the R.txt file should contain a '
      'list of resources to be included in the R.java file in the format '
      'generated by aapt')

  parser.add_option(
      '--all-resources-zip-out',
      help='Path for output of all resources. This includes resources in '
      'dependencies.')

  parser.add_option('--stamp', help='File to touch on success')

  (options, args) = parser.parse_args(args)

  if args:
    parser.error('No positional arguments should be given.')

  # Check that required options have been provided.
  required_options = (
      'android_sdk',
      'android_sdk_tools',
      'android_manifest',
      'dependencies_res_zips',
      'resource_dirs',
      'resource_zip_out',
      )
  build_utils.CheckOptions(options, parser, required=required_options)

  if (options.R_dir is None) == (options.srcjar_out is None):
    raise Exception('Exactly one of --R-dir or --srcjar-out must be specified.')

  return options


def CreateExtraRJavaFiles(
    r_dir, extra_packages, extra_r_text_files):
  if len(extra_packages) != len(extra_r_text_files):
    raise Exception('--extra-res-packages and --extra-r-text-files'
                    'should have the same length')

  java_files = build_utils.FindInDirectory(r_dir, "R.java")
  if len(java_files) != 1:
    return
  r_java_file = java_files[0]
  r_java_contents = open(r_java_file).read()

  for package in extra_packages:
    package_r_java_dir = os.path.join(r_dir, *package.split('.'))
    build_utils.MakeDirectory(package_r_java_dir)
    package_r_java_path = os.path.join(package_r_java_dir, 'R.java')
    open(package_r_java_path, 'w').write(
        re.sub(r'package [.\w]*;', 'package %s;' % package, r_java_contents))
    # TODO(cjhopman): These extra package's R.java files should be filtered to
    # only contain the resources listed in their R.txt files. At this point, we
    # have already compiled those other libraries, so doing this would only
    # affect how the code in this .apk target could refer to the resources.


def DidCrunchFail(returncode, stderr):
  """Determines whether aapt crunch failed from its return code and output.

  Because aapt's return code cannot be trusted, any output to stderr is
  an indication that aapt has failed (http://crbug.com/314885), except
  lines that contain "libpng warning", which is a known non-error condition
  (http://crbug.com/364355).
  """
  if returncode != 0:
    return True
  for line in stderr.splitlines():
    if line and not 'libpng warning' in line:
      return True
  return False


def ZipResources(resource_dirs, zip_path):
  # Python zipfile does not provide a way to replace a file (it just writes
  # another file with the same name). So, first collect all the files to put
  # in the zip (with proper overriding), and then zip them.
  files_to_zip = dict()
  for d in resource_dirs:
    for root, _, files in os.walk(d):
      for f in files:
        archive_path = os.path.join(os.path.relpath(root, d), f)
        path = os.path.join(root, f)
        files_to_zip[archive_path] = path
  with zipfile.ZipFile(zip_path, 'w') as outzip:
    for archive_path, path in files_to_zip.iteritems():
      outzip.write(path, archive_path)


def main():
  args = build_utils.ExpandFileArgs(sys.argv[1:])

  options = ParseArgs(args)
  android_jar = os.path.join(options.android_sdk, 'android.jar')
  aapt = os.path.join(options.android_sdk_tools, 'aapt')

  input_files = []

  with build_utils.TempDir() as temp_dir:
    deps_dir = os.path.join(temp_dir, 'deps')
    build_utils.MakeDirectory(deps_dir)
    v14_dir = os.path.join(temp_dir, 'v14')
    build_utils.MakeDirectory(v14_dir)

    gen_dir = os.path.join(temp_dir, 'gen')
    build_utils.MakeDirectory(gen_dir)

    input_resource_dirs = build_utils.ParseGypList(options.resource_dirs)

    for resource_dir in input_resource_dirs:
      generate_v14_compatible_resources.GenerateV14Resources(
          resource_dir,
          v14_dir,
          options.v14_verify_only)

    dep_zips = build_utils.ParseGypList(options.dependencies_res_zips)
    input_files += dep_zips
    dep_subdirs = []
    for z in dep_zips:
      subdir = os.path.join(deps_dir, os.path.basename(z))
      if os.path.exists(subdir):
        raise Exception('Resource zip name conflict: ' + os.path.basename(z))
      build_utils.ExtractAll(z, path=subdir)
      dep_subdirs.append(subdir)

    # Generate R.java. This R.java contains non-final constants and is used only
    # while compiling the library jar (e.g. chromium_content.jar). When building
    # an apk, a new R.java file with the correct resource -> ID mappings will be
    # generated by merging the resources from all libraries and the main apk
    # project.
    package_command = [aapt,
                       'package',
                       '-m',
                       '-M', options.android_manifest,
                       '--auto-add-overlay',
                       '-I', android_jar,
                       '--output-text-symbols', gen_dir,
                       '-J', gen_dir]

    for d in input_resource_dirs:
      package_command += ['-S', d]

    for d in dep_subdirs:
      package_command += ['-S', d]

    if options.non_constant_id:
      package_command.append('--non-constant-id')
    if options.custom_package:
      package_command += ['--custom-package', options.custom_package]
    if options.proguard_file:
      package_command += ['-G', options.proguard_file]
    build_utils.CheckOutput(package_command, print_stderr=False)

    if options.extra_res_packages:
      CreateExtraRJavaFiles(
          gen_dir,
          build_utils.ParseGypList(options.extra_res_packages),
          build_utils.ParseGypList(options.extra_r_text_files))

    # This is the list of directories with resources to put in the final .zip
    # file. The order of these is important so that crunched/v14 resources
    # override the normal ones.
    zip_resource_dirs = input_resource_dirs + [v14_dir]

    base_crunch_dir = os.path.join(temp_dir, 'crunch')

    # Crunch image resources. This shrinks png files and is necessary for
    # 9-patch images to display correctly. 'aapt crunch' accepts only a single
    # directory at a time and deletes everything in the output directory.
    for idx, d in enumerate(input_resource_dirs):
      crunch_dir = os.path.join(base_crunch_dir, str(idx))
      build_utils.MakeDirectory(crunch_dir)
      zip_resource_dirs.append(crunch_dir)
      aapt_cmd = [aapt,
                  'crunch',
                  '-C', crunch_dir,
                  '-S', d]
      build_utils.CheckOutput(aapt_cmd, fail_func=DidCrunchFail)

    ZipResources(zip_resource_dirs, options.resource_zip_out)

    if options.all_resources_zip_out:
      ZipResources(
          zip_resource_dirs + dep_subdirs, options.all_resources_zip_out)

    if options.R_dir:
      build_utils.DeleteDirectory(options.R_dir)
      shutil.copytree(gen_dir, options.R_dir)
    else:
      build_utils.ZipDir(options.srcjar_out, gen_dir)

  if options.depfile:
    input_files += build_utils.GetPythonDependencies()
    build_utils.WriteDepfile(options.depfile, input_files)

  if options.stamp:
    build_utils.Touch(options.stamp)


if __name__ == '__main__':
  main()