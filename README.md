# RISCV-Pipeline
RISC-V Pipeline Simulator
Overview

This project implements a cycle-accurate simulator for a RISC-V pipeline with a focus on understanding the hardware architecture design of the RISC-V CPU. The simulator will handle instruction execution in a 5-stage single-issue pipeline, including handling data and control hazards.

The project is divided into two major milestones:

Milestone 1: Basic 5-stage pipeline without hazard detection or resolution.
Milestone 2: Full pipeline with hazard detection and resolving, using techniques like data forwarding and stalling.
Milestones

Milestone 1: Basic Pipeline

In this milestone, we implement a 5-stage RISC-V pipeline that simulates instruction execution through each stage of the pipeline. The stages are:

Instruction Fetch (IF)
Instruction Decode (ID)
Execute (EX)
Memory Access (MEM)
Write-Back (WB)

Key objectives:

Simulate the movement of instructions through the pipeline.
Track and print which instruction is processed at each pipeline stage every clock cycle.
Simulate the basic execution without handling hazards.

Testing Milestone 1:

Cycle Counter: Tracks the number of clock cycles.
Instruction Trace: Prints the instruction processed at each stage.
Register Trace: Dumps the register file for every cycle.
Milestone 2: Full Pipeline with Hazard Detection/Resolution

This milestone extends Milestone 1 by introducing hazard detection and hazard resolution:

Data Hazards: Detected and resolved using data forwarding and stalls.
Control Hazards: Handled by flushing the pipeline when a branch is taken.

Key objectives:

Implement hazard detection and resolve them via stalling and data forwarding.
Track the number of stalls in the pipeline.

Testing Milestone 2:

Hazard Detection: Verify that data and control hazards are detected and resolved.
Pipeline Simulation: Simulate and print the cycle-by-cycle execution and hazard resolution.
Files in the Project
Milestone 1
pipeline.c: Contains the core simulation function cycle_pipeline, simulating one cycle of the pipeline.
pipeline.h: Declares the data structures and function prototypes for the simulator.
stage_helpers.h: Contains helper functions for executing each stage of the pipeline.
disasm.c: Used for decoding instructions.
utils.c & emulator.c: Replaced with your own versions from Lab 2-3.
Milestone 2
pipeline.c: Extended to handle hazard detection and resolution in the pipeline.
pipeline.h: Updated to include structures for hazard detection and control signals.
stage_helpers.h: Contains functions for detecting and resolving data/control hazards.
cache.c & cache.h: For simulating cache memory and configurations.
How to Run
Milestone 1

R-type Instructions:

./riscv -s -v ./code/ms1/input/R/R.input > ./code/ms1/out/R/R.trace
diff ./code/ms1/ref/R/R.trace ./code/ms1/out/R/R.trace

I-type Instructions:

./riscv -s -v ./code/ms1/input/I/I.input > ./code/ms1/out/I/I.trace
diff ./code/ms1/ref/I/I.trace ./code/ms1/out/I/I.trace

Load/Store Instructions:

./riscv -s -v ./code/ms1/input/LS/LS.input > ./code/ms1/out/LS/LS.trace
diff ./code/ms1/ref/LS/LS.trace ./code/ms1/out/LS/LS.trace

Multiply Test:

./riscv -s -v ./code/ms1/input/multiply.input > ./code/ms1/out/multiply.trace
diff ./code/ms1/ref/multiply.trace ./code/ms1/out/multiply.trace

Random Test:

./riscv -s -v ./code/ms1/input/random.input > ./code/ms1/out/random.trace
diff ./code/ms1/ref/random.trace ./code/ms1/out/random.trace
Milestone 2

To test Milestone 2, use the following command:

./riscv -s -e -f ./code/ms2/input/vec_xprod.input > ./code/ms2/out/vec_xprod.trace
diff ./code/ms2/ref/vec_xprod.trace ./code/ms2/out/vec_xprod.trace
Setup and Requirements
C++ Compiler: You need a C++ compiler that supports C++11 or later.
Linux Environment: Ensure the code compiles and runs correctly on the lab computers (FAS-RLA Linux).
Project Structure
/riscv-pipeline
    /milestone1/
        - pipeline.c
        - pipeline.h
        - stage_helpers.h
        - disasm.c
        - utils.c
        - emulator.c
    /milestone2/
        - pipeline.c
        - pipeline.h
        - stage_helpers.h
        - cache.c
        - cache.h
        - test_simulator_ms2.sh
    README.md
