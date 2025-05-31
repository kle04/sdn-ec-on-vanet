# Applying Edge Computing and SDN in VANET

## Overview
This repository contains the final course project for the Computer Networking Department at UIT-VNUHCM. The project focuses on enhancing the performance and reducing the latency of **Vehicular Ad-hoc Networks (VANETs)** through the integration of **Edge Computing** and **Software-Defined Networking (SDN)**.

**Instructors:** ThS. Nguyễn Khánh Thuật, ThS. Văn Thiên Luân  
**Project Duration:** February 2025 – June 2025  
**Contributors:**
- Lê Hữu Khánh - 22520636  
- Nguyễn Quang Dũng - 22520287

## Table of Contents
- [Objectives](#objectives)
- [Architecture & Scenario](#architecture--scenario)
- [Technologies](#technologies)
- [Simulation Environment](#simulation-environment)
- [Performance Metrics](#performance-metrics)
- [Results (Expected)](#results-expected)
- [References](#references)

## Objectives
- Build and simulate an integrated VANET architecture enhanced by Edge Computing and SDN.
- Compare traditional routing protocols (AODV, OLSR) with SDN-based VANET.
- Evaluate network performance based on latency, throughput, and packet delivery ratio (PDR).

## Architecture & Scenario
- **V2V (Vehicle-to-Vehicle):** Direct communication between vehicles to exchange warnings and real-time data.
- **V2I (Vehicle-to-Infrastructure):** Vehicles communicate with Road Side Units (RSU) and MEC Servers for faster decision-making.
- **SDN Controller:** Centralized network management, flexible route optimization, and traffic-aware configurations.

## Technologies
- **Simulation Tool:** NS-3 (Network Simulator 3)
- **Visualization:** Python (Matplotlib)
- **Protocols:** AODV, OLSR, IEEE 802.11p
- **Architecture Components:** SDN Controller, MEC Server, RSU, OBU

## Simulation Environment
- Number of nodes: 40
- Simulation time: 100 seconds (movement: 60s)
- Movement speed: 5 m/s
- Area: 500m x 500m
- Data Rate: 250 Kbps
- Protocol: UDP traffic

## Performance Metrics
- **Throughput:** Total data received per unit time
- **Average Delay:** Mean time for packets to travel from source to destination
- **Packet Delivery Ratio (PDR):** Ratio of successfully delivered packets to total sent

## Results (Expected)
- SDN-based VANET reduces latency by ~30% and increases throughput by ~25% compared to traditional VANET.
- Improved responsiveness in real-time traffic scenarios.
- Centralized control via SDN enables dynamic path adjustment and congestion mitigation.

## References
- [MedHocNet 2014 - Towards Software Defined VANET](https://doi.org/10.1109/MedHocNet.2014.6849111)
- Journal of Electrical Systems, 2024 – Emerging trends in SDN-VANET
- [Vanet simulators: an updated review](https://doi.org/10.1186/s13173-021-00113-x)
- World Health Organization, “Global status report on road safety 2018,” World Health Organization,
Tech. Rep., 2018.
- K. Gaur and J. Grover, “Exploring vanet using edge computing and sdn,” in 2019 Second International
Conference on Advanced Computational and Communication Paradigms (ICACCP), 2019, pp. 1–4.

> See `/docs` folder for full reports and diagrams.
