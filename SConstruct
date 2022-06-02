# Build utilities specific to this project, in the `site_scons` folder
import firmware.platforms
import firmware.targets
import versions
import os
from pprint import pprint

env = Environment(tools=['vscode_c_cpp_properties'])
vscode_configurations = []

# Read PS2+ versions from the `ps2plus.json` manifest (with command-line overrides)
# and add them to the C compilation environment
ps2plus_versions = versions.setup_env(env)
pprint(ps2plus_versions)

# Build the configurator application (by calling `make`)
if 'dist/configurator' in BUILD_TARGETS:
    env.AlwaysBuild(env.Command(
        target='dist/configurator', 
        source='configurator', 
        action=f'make -C configurator ROOTDIR=`pwd` PROJECTDIR="$SOURCE" DISTDIR="$TARGET" VERSION="{ps2plus_versions.configurator}"',
        ENV=os.environ,
    ))

# Build the firmware/bootloader for each platform
if 'dist/firmware' in BUILD_TARGETS:
    for platform in firmware.platforms.ALL_PLATFORMS:
        firmware_outputs = {}
        firmware_sources = set()
        firmware_dists = {}

        # Create a construction environment for this platform
        platform_env = env.Clone(tools=(env['TOOLS'] + platform.toolchain.tools))
        platform.toolchain.setup_env(platform_env)

        for target in firmware.targets.ALL_TARGETS:
            # Create a construction environment for this target
            build_env = platform_env.Clone()
            platform.setup_env_for_target(build_env, target)
            
            # Build the `firmware` sub-project, passing in platform and target information
            # (e.g.: `PIC18F46K42`` platform and `bootloader` target). Sub-project will build
            # its targets into the `build/{PLATFORM}/{TARGET}` folder.
            output, sources, vscode_configuration = SConscript('firmware/SConscript', 
                variant_dir=os.path.join('build', platform.name, target), 
                duplicate=False, 
                exports=['build_env', 'platform', 'target']
            )

            firmware_outputs[target] = output
            firmware_sources |= set([os.path.join('firmware', path) for path in sources])
            vscode_configurations.append(vscode_configuration)

        # Move each target's output file to the `dist` folder
        for target, output in firmware_outputs.items():
            firmware_dists[target] = InstallAs(
                target=f'dist/firmware/ps2plus-{platform.name}-{target}-{getattr(ps2plus_versions, target)}.hex', 
                source=output
            )

        # Merge the different targets into a single combined image
        combined_version = '-'.join([f'{target}-{getattr(ps2plus_versions, target)}' for target in firmware_dists.keys()])
        combined_target = platform_env.Command(
            target=f'dist/firmware/ps2plus-{platform.name}-combined-{combined_version}.hex',
            source=firmware_dists.values(),
            action='python3 tools/hex-tools/merge.py --overlap=ignore $SOURCES -o $TARGET'
        )

        # ZIP up all platform files
        Zip(
            target=f'dist/firmware/ps2plus-{platform.name}-{combined_version}.zip',
            source=(list(firmware_dists.values()) + [combined_target])
        )

        # Generate the IDE project(s), if any, for this platform
        platform.generate_ide_project(platform_env, f'firmware/build-projects/{platform.name}', firmware_sources)

# Generate VS Code autocompletion
env.AlwaysBuild(env.VSCodeCCppProperties('.vscode/c_cpp_properties.json', vscode_configurations))