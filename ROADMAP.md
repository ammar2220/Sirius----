# Sirius Project Roadmap

## Phase 0: Project Organization & Persistent Memory
- [x] Establish project folder structure
- [x] Create `PROJECT_MEMORY.md` source of truth
- [x] Document existing ESP32 prototype baseline
- [x] Document verified testing history
- [x] Define session recovery rules
**STATUS: DONE**

## Phase 1: AION UX Analysis
- [x] Install AION APK on Android emulator
- [x] Static analysis of APK bundle
- [x] Dynamic analysis of network activity
- [x] Identify identity/provisioning concepts (USID, Rooms)
- [x] Determine cloud-based control architecture
**STATUS: DONE**

## Phase 2: Sirius UX Specification
- [x] Define core interaction principles
- [x] Map onboarding flow (Provisioning $\rightarrow$ Registration)
- [x] Design room and device organization
- [x] Define device control interface (ON/OFF, State)
- [x] Create UX specification document
**STATUS: DONE**

## Phase 3: Mobile Application Architecture
- [x] Select technology stack (React Native CLI)
- [x] Design backend abstraction (Auth/Data interfaces)
- [x] Define user authentication and member model
- [x] Plan device registration and provisioning flow
**STATUS: DONE**

## Phase 4: Sirius Mobile Application MVP
- [ ] Implement login/signup
- [ ] Implement "Add Device" UI
- [ ] Implement basic device control screen (In Progress - APK building)
- [ ] Implement room organization
**STATUS: IN PROGRESS**

## Phase 5: ESP32 $\leftrightarrow$ Sirius Local Communication
- [ ] Define Sirius local API contract
- [ ] Implement local API in ESP32 firmware
- [ ] Implement local discovery (mDNS)
- [ ] Verify app-to-device local control
**STATUS: FUTURE**

## Phase 6: Provisioning Flow
- [ ] Implement secure provisioning handshake
- [ ] Automate device registration with Sirius backend
- [ ] Verify end-to-end onboarding
**STATUS: FUTURE**

## Phase 7: Real Hardware End-to-End Testing
- [ ] Test with final PCB / Enclosure
- [ ] Verify mechanical relay performance
- [ ] Test capacitive touch responsiveness
- [ ] Validate thermal performance under load
**STATUS: FUTURE**

## Phase 8: Product Hardening
- [ ] Implement robust error handling
- [ ] Add watchdog timers and auto-recovery
- [ ] Optimize power consumption
- [ ] Perform stress tests
**STATUS: FUTURE**

## Phase 9: Cloud/Backend Architecture
- [ ] Implement scalable cloud broker
- [ ] Enable remote control (outside local network)
- [ ] Implement push notifications
- [ ] Set up device history/analytics
**STATUS: FUTURE**

## Phase 10: Commercial Product Preparation
- [ ] Finalize manufacturing specs
- [ ] Quality assurance testing
- [ ] Regulatory compliance check
- [ ] Packaging and documentation
**STATUS: FUTURE**
