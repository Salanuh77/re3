Feature: Physics is independent of rendered frames
  The original handling runs at 30 fixed ticks per second while rendering is interpolated.

  Scenario Outline: Equal wall time produces equal physics at different display rates
    Given a reference simulation rendered at 30 FPS
    When the simulation runs for 10 seconds at <fps> FPS
    Then exactly 300 physics ticks have run
    And its position and velocity match the reference
    And the interpolation fraction is between zero and one

    Examples:
      | fps  |
      | 15   |
      | 30   |
      | 60   |
      | 120  |
      | 144  |
      | 240  |
      | 1000 |

  Scenario: Pausing discards accumulated wall time
    When a 240 FPS simulation is paused for 20 seconds
    Then exactly 300 physics ticks have run
    And the interpolation fraction is zero

  Scenario: A long stall cannot trigger unlimited catch-up
    When a 240 FPS simulation stalls for 10 seconds
    Then exactly 305 physics ticks have run
    And no render frame runs more than 5 physics ticks
    And the interpolation fraction is between zero and one

  Scenario: Slow motion scales time deposited into the accumulator
    When a 240 FPS simulation runs at half speed
    Then exactly 150 physics ticks have run

  Scenario: Rendering blends between completed physics ticks
    When a transform at position 10 is rendered with interpolation fraction 0.5
    Then its displayed position is 5
    And its authoritative physics position remains 10
    And its displayed rotation is normalized

  Scenario: Teleports snap instead of sweeping through the world
    When a transform at position 1000 is rendered with interpolation fraction 0.5
    Then its displayed position is 1000
    And its authoritative physics position remains 1000
