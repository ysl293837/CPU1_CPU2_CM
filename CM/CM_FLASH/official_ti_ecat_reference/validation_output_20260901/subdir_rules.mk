################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
official_ti_ecat_reference/validation_output_20260901/%.obj: ../official_ti_ecat_reference/validation_output_20260901/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Arm Compiler - building file: "$<"'
	"C:/ti/ti-cgt-arm_20.2.7.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=none -me -Ooff --include_path="C:/Users/yangshilin/Desktop/Embedded Learning/DSP_PMSM_project/TEST/me/c28388_4.0/Dual_CPU/CM" --include_path="C:/Users/yangshilin/Desktop/Embedded Learning/DSP_PMSM_project/TEST/me/c28388_4.0/Dual_CPU/CM/device" --include_path="C:/ti/c2000/C2000Ware_26_00_00_00/driverlib/f2838x/driverlib_cm/" --include_path="C:/ti/ti-cgt-arm_20.2.7.LTS/include" --define=DEBUG --define=_FLASH --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --preproc_with_compile --preproc_dependency="official_ti_ecat_reference/validation_output_20260901/$(basename $(<F)).d_raw" --obj_directory="official_ti_ecat_reference/validation_output_20260901" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


