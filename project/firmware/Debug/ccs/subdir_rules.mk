################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
ccs/%.obj: ../ccs/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: ARM Compiler'
	"/Applications/ti/ccs920/ccs/tools/compiler/ti-cgt-arm_18.12.3.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me --include_path="/Users/mrinmoysarkar/Documents/git_repos/embedded_system/project/firmware" --include_path="/Users/mrinmoysarkar/ti/simplelink_msp432p4_sdk_3_30_00_13/source" --include_path="/Users/mrinmoysarkar/ti/simplelink_msp432p4_sdk_3_30_00_13/source/third_party/CMSIS/Include" --include_path="/Applications/ti/ccs920/ccs/tools/compiler/ti-cgt-arm_18.12.3.LTS/include" --advice:power=none --define=__MSP432P401R__ --define=DeviceFamily_MSP432P401x -g --diag_warning=225 --diag_warning=255 --diag_wrap=off --display_error_number --gen_func_subsections=on --preproc_with_compile --preproc_dependency="ccs/$(basename $(<F)).d_raw" --obj_directory="ccs" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


