# To run makemake as a final step in the build...
# 1. Add a target to Settings/Target with the following attributes:
#	 File Extension:	sh
#	 Tool Name:			sh
#	 Flags:				PostLink Stage
# 2. Add this file to your project
# After each build, this script will be run

/boot/develop/BeIDE/tools/makemake > /dev/null
