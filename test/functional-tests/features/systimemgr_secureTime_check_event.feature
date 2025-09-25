##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2018 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################

Feature: Ensures /tmp/systimemgr/drm file is available.

  Scenario: Ensures SystemTimeManager Initialization
    Given the SystemTimeManager is not already running
    When the SystemTimeManager binary is invoked
    Then the SystemTimeManager should be started

  Scenario: Check if systemTimeManager is Running
    Given the SystemTimeManager is running
    When the SystemTimeManager binary is invoked
    Then the SystemTimeManager should be running

  Scenario: Verify SystemTimeManager Log file exists
    Given the SystemTimeManager is running
    When the SystemTimeManager binary is invoked
    Then the SystemTimeManager LogFile should get generated

  Scenario: Verify That /tmp/systimemgr/drm file is created/modified.
    Given the SystemTimeManager is running
    When the Secure Time is available
    Then the SystemTimeManager should create/modify the /tmp/systimemgr/drm file.

  Scenario: Verify the Event should be eSYSMGR_EVENT_SECURE_TIME_AVAILABLE.
     Given the SystimeTimeManager is running
     When the Secure Time is available
     Then the Event eSYSMGR_EVENT_SECURE_TIME_AVAILABLE should be found and Secure Message should be published.
