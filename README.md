# Morphing Wheel Rover – Semester Project (CRATE Lab, EPFL)

This repository contains the source code, experimental datasets, and analysis tools developed during a semester project conducted at the CRATE Lab (EPFL). The project investigates the performance of a morphing-wheel rover capable of operating in multiple wheel configurations and evaluates the impact of wheel geometry on mobility, energy consumption, payload-carrying capability, and obstacle-climbing performance.

## Repository Contents

### Experimental Data

The repository includes the datasets collected during the experimental campaign performed on the rover.

The data are divided into two main categories:

* **Baseline experiments (without payload):** measurements acquired for the three wheel configurations on various terrains, including flat ground, gravel, slopes, grass, sand, and shell-covered surfaces.
* **Payload experiments:** measurements obtained while carrying additional payloads of 0.5 kg, 0.89 kg, and 1.0 kg. These tests were performed on a subset of representative terrains to evaluate the influence of increasing mass on the rover's energy consumption and locomotion capabilities.

For each experiment, electrical measurements were recorded using an INA219 current and voltage sensor. The resulting datasets contain quantities such as current, voltage, power consumption, and cumulative energy consumption over time.

### Data Analysis

The repository contains Jupyter notebooks used to process and analyze the experimental data.

These notebooks provide:

* Data cleaning and preprocessing routines.
* Computation of performance metrics such as average current, average power, energy consumption, and Cost of Transport (CoT).
* Generation of figures and plots used in the project report.
* Comparison of wheel configurations across terrains and payload conditions.
* Statistical summaries and visualization tools for interpreting rover performance.

### Arduino / ESP32 Code

Several versions of the embedded software developed during the project are included in this repository.

These Arduino sketches correspond to different stages of the development process and include:

* Initial sensor acquisition tests using the INA219 current sensor.
* Serial-based data logging implementations.
* Wi-Fi-enabled versions allowing remote control of the rover through a web interface.
* Data recording and CSV export functionalities.
* Motor control experiments and integration with external switching hardware.
* Live monitoring and visualization prototypes developed throughout the project.

The different iterations have been preserved to document the evolution of the system and facilitate future development.

## Project Objective

The primary objective of this project was to evaluate the advantages and limitations of a morphing-wheel concept by experimentally comparing several wheel configurations under realistic operating conditions. Particular attention was given to locomotion efficiency, payload transport, obstacle-crossing capability, and overall system robustness.

The results presented in this repository form the basis of the analysis and conclusions reported in the final semester project report.
