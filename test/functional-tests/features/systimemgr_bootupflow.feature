Feature: Ensures SystemTimeManager Bootup Flow

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

  Scenario: Verify SystemTimeManager Instance is created
    Given the SystemTimeManager is running
    When the SystemTimeManager binary is invoked
    Then the SystemTimeManager Instance should get created

  Scenario: Verify SystemTimeManager Initializes timeSrc and timeSync
    Given the SystemTimeManager is running
    When the SystemTimeManager binary is invoked
    Then the SystemTimeManager should initializes TimeSrc[NTP,DTT] and TimeSync[clocktime]

  Scenario: Verify SystemTimeManager returns the last known good time
    Given the SystemTimeManager is running
    When the SystemTimeManager registers for IARM events
    Then the SystemTimeManager should return the last known good time
    And the log file should contain "Returning Last Known Good Time"

  Scenario: Verify SystemTimeManager Sends info about time Quality
    Given the SystemTimeManager is running
    When the SystemTimeManager returns the last known good time
    Then the SystemTimeManager should Send broadcast msg about the time quality
    And the log file should contain "Info:"
