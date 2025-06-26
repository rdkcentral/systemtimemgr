Feature: Ensures SystemTimeManager generates /opt/secure/clock.txt File

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

  Scenario: Verify SystemTimeManager generates clock.txt File
    Given the SystemTimeManager is running
    When the SystemTimeManager returns the last known time
    Then the SystemTimeManager should update the clock.txt File
