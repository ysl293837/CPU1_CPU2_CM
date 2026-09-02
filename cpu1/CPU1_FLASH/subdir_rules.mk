################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
build-986373841: ../c2000_cpu1.syscfg
	@echo 'SysConfig - building file: "$<"'
	"E:/CCS20/CCS20.5.0/ccs/utils/sysconfig_1.27.0/sysconfig_cli.bat" --script "C:/Users/yangshilin/Desktop/Embedded Learning/DSP_PMSM_project/TEST/me/c28388_4.0/Dual_CPU/cpu1/c2000_cpu1.syscfg" -o "syscfg" --product "C:/ti/c2000/C2000Ware_26_00_00_00/.metadata/sdk.json" --compiler ccs
	@echo 'Finished building: "$<"'
	@echo ' '

syscfg/board.c: build-986373841 ../c2000_cpu1.syscfg
syscfg/board.h: build-986373841
syscfg/board.cmd.genlibs: build-986373841
syscfg/board.opt: build-986373841
syscfg/board.json: build-986373841
syscfg/pinmux.csv: build-986373841
syscfg/epwm.dot: build-986373841
syscfg/device.c: build-986373841
syscfg/device.h: build-986373841
syscfg/adc.dot: build-986373841
syscfg/c2000ware_libraries.cmd.genlibs: build-986373841
syscfg/c2000ware_libraries.opt: build-986373841
syscfg/c2000ware_libraries.c: build-986373841
syscfg/c2000ware_libraries.h: build-986373841
syscfg/clocktree.h: build-986373841
syscfg: build-986373841

syscfg/%.obj: ./syscfg/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'C2000 Compiler - building file: "$<"'
	"E:/CCS20/CCS20.5.0/ccs/tools/compiler/ti-cgt-c2000_25.11.0.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla2 --float_support=fpu64 --idiv_support=idiv0 --tmu_support=tmu0 --vcu_support=vcrc -Ooff --include_path="C:/Users/yangshilin/Desktop/Embedded Learning/DSP_PMSM_project/TEST/me/c28388_4.0/Dual_CPU/cpu1" --include_path="C:/Users/yangshilin/Desktop/Embedded Learning/DSP_PMSM_project/TEST/me/c28388_4.0/Dual_CPU/cpu1/CPU1_FLASH/syscfg" --include_path="C:/Users/yangshilin/Desktop/Embedded Learning/DSP_PMSM_project/TEST/me/c28388_4.0/Dual_CPU/cpu1/device" --include_path="C:/ti/c2000/C2000Ware_26_00_00_00/driverlib/f2838x/driverlib/" --include_path="C:/Users/yangshilin/Desktop/Embedded Learning/DSP_PMSM_project/TEST/me/c28388_4.0/Dual_CPU/cpu1/APP/pc_serial" --include_path="C:/Users/yangshilin/Desktop/Embedded Learning/DSP_PMSM_project/TEST/me/c28388_4.0/Dual_CPU/cpu1/APP/key" --include_path="C:/Users/yangshilin/Desktop/Embedded Learning/DSP_PMSM_project/TEST/me/c28388_4.0/Dual_CPU/cpu1/APP/Closed_loop" --include_path="E:/CCS20/CCS20.5.0/ccs/tools/compiler/ti-cgt-c2000_25.11.0.LTS/include" --define=_FLASH --define=DEBUG --define=CPU1 --diag_suppress=10063 --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --abi=eabi --preproc_with_compile --preproc_dependency="syscfg/$(basename $(<F)).d_raw" --include_path="C:/Users/yangshilin/Desktop/Embedded Learning/DSP_PMSM_project/TEST/me/c28388_4.0/Dual_CPU/cpu1/CPU1_FLASH/syscfg" --obj_directory="syscfg" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

%.obj: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'C2000 Compiler - building file: "$<"'
	"E:/CCS20/CCS20.5.0/ccs/tools/compiler/ti-cgt-c2000_25.11.0.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla2 --float_support=fpu64 --idiv_support=idiv0 --tmu_support=tmu0 --vcu_support=vcrc -Ooff --include_path="C:/Users/yangshilin/Desktop/Embedded Learning/DSP_PMSM_project/TEST/me/c28388_4.0/Dual_CPU/cpu1" --include_path="C:/Users/yangshilin/Desktop/Embedded Learning/DSP_PMSM_project/TEST/me/c28388_4.0/Dual_CPU/cpu1/CPU1_FLASH/syscfg" --include_path="C:/Users/yangshilin/Desktop/Embedded Learning/DSP_PMSM_project/TEST/me/c28388_4.0/Dual_CPU/cpu1/device" --include_path="C:/ti/c2000/C2000Ware_26_00_00_00/driverlib/f2838x/driverlib/" --include_path="C:/Users/yangshilin/Desktop/Embedded Learning/DSP_PMSM_project/TEST/me/c28388_4.0/Dual_CPU/cpu1/APP/pc_serial" --include_path="C:/Users/yangshilin/Desktop/Embedded Learning/DSP_PMSM_project/TEST/me/c28388_4.0/Dual_CPU/cpu1/APP/key" --include_path="C:/Users/yangshilin/Desktop/Embedded Learning/DSP_PMSM_project/TEST/me/c28388_4.0/Dual_CPU/cpu1/APP/Closed_loop" --include_path="E:/CCS20/CCS20.5.0/ccs/tools/compiler/ti-cgt-c2000_25.11.0.LTS/include" --define=_FLASH --define=DEBUG --define=CPU1 --diag_suppress=10063 --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --abi=eabi --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" --include_path="C:/Users/yangshilin/Desktop/Embedded Learning/DSP_PMSM_project/TEST/me/c28388_4.0/Dual_CPU/cpu1/CPU1_FLASH/syscfg" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


