local architectures = ['X64', 'ARM64'];

local imagemap = {
  'centos-9': 'almalinux:9',
  'centos-10': 'almalinux:10',
  'debian-bookworm': 'debian:bookworm',
  'debian-trixie': 'debian:trixie',
  'ubuntu-jammy': 'ubuntu:22.04',
  'ubuntu-noble': 'ubuntu:24.04',
  'ubuntu-resolute': 'ubuntu:26.04',
};

local build_pipeline = {
  name: 'build',
  on: {
    workflow_call: {
      inputs: {
        version: {
          required: true,
          type: 'string',
        },
        experimental: {
          required: false,
          type: 'boolean',
        },
        branch: {
          required: false,
          type: 'string',
        },
        repo: {
          required: false,
          type: 'string',
        },
        distributions: {
          required: false,
          type: 'string',
        },
        architectures: {
          required: false,
          type: 'string',
        },
        asan: {
          required: false,
          type: 'boolean',
        },
      },
    },
  },
};

local include_distro(name) =
  'inputs.distributions == \'\' || contains(format(\',{0},\', inputs.distributions), format(\',{0},\', \'' + name + '\'))';

local include_arch(arch) =
  'inputs.architectures == \'\' || contains(format(\',{0},\', inputs.architectures), format(\',{0},\', \'' + arch + '\'))';

local build_test_jobs(name) = {
  local build_with(arch) = {
    name: name,
    platform: arch,
    version: '${{ inputs.version }}',
    experimental: '${{ inputs.experimental }}',
    branch: '${{ inputs.branch }}',
    repo: '${{ inputs.repo }}',
    asan: '${{ inputs.asan }}',
  },
  [name + '-build-' + arch]: {
    'if': '${{ (' + include_distro(name) + ') && (' + include_arch(arch) + ') }}',
    uses: './.github/workflows/build_packages.yml',
    with: build_with(arch),
  }
  for arch in architectures
};

local build_jobs_list = [
  build_test_jobs(name)
  for name in std.objectFields(imagemap)
];

local all_jobs = {
  jobs:
    std.foldl(std.mergePatch, build_jobs_list, {})
};

{
  'build.yml': build_pipeline + all_jobs,
}
