################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
CPU1_RAM/syscfg/%.obj: ../CPU1_RAM/syscfg/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'C2000 Compiler - building file: "$<"'
	"E:/CCS20/CCS20.5.0/ccs/tools/compiler/ti-cgt-c2000_25.11.0.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla2 --float_support=fpu64 --idiv_support=idiv0 --tmu_support=tmu0 --vcu_support=vcrc -Ooff --include_path="C:/Users/yangshilin/Desktop/DSP_PMSM_project/TEST/me/c28388_4.0/Dual_CPU/Driverlib Empty Dual Example CCS Project" --include_path="C:/Users/yangshilin/Desktop/DSP_PMSM_project/TEST/me/c28388_4.0/Dual_CPU/Driverlib Empty Dual Example CCS Project/device" --include_path="C:/ti/c2000/C2000Ware_26_00_00_00/driverlib/f2838x/driverlib/" --include_path="C:/Users/yangshilin/Desktop/DSP_PMSM_project/TEST/me/c28388_4.0/Dual_CPU/Driverlib Empty Dual Example CCS Project/APP/pc_serial" --include_path="C:/Users/yangshilin/Desktop/DSP_PMSM_project/TEST/me/c28388_4.0/Dual_CPU/Driverlib Empty Dual Example CCS Project/APP/key" --include_path="C:/Users/yangshilin/Desktop/DSP_PMSM_project/TEST/me/c28388_4.0/Dual_CPU/Driverlib Empty Dual Example CCS Project/APP/Closed_loop" --include_path="E:/CCS20/CCS20.5.0/ccs/tools/compiler/ti-cgt-c2000_25.11.0.LTS/include" --define=_FLASH --define=DEBUG --define=CPU1 --diag_suppress=10063 --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --abi=eabi --preproc_with_compile --preproc_dependency="CPU1_RAM/syscfg/$(basename $(<F)).d_raw" --include_path="C:/Users/yangshilin/Desktop/DSP_PMSM_project/TEST/me/c28388_4.0/Dual_CPU/Driverlib Empty Dual Example CCS Project/CPU1_FLASH/syscfg" --obj_directory="CPU1_RAM/syscfg" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


