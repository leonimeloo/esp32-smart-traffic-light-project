# Traffic Control Logic

This document describes the adaptive traffic control algorithm implemented by the main ESP32.

The system determines which road should receive priority based on both the number of vehicles and the amount of time each queue has been waiting.

---

## 1. Objective

The objective of the control algorithm is to avoid relying on a completely fixed traffic light cycle.

Instead, the controller continuously evaluates the traffic demand of the two roads and determines which queue should receive priority.

The algorithm considers two variables:

```text
Vehicle Count
      +
Waiting Time
      |
      v
Queue Priority
```

---

## 2. Queue Priority

The prototype calculates the queue priority using:

```text
P = 5 × N + T
```

Where:

| Variable | Description |
|---|---|
| `P` | Queue priority |
| `N` | Number of detected vehicles |
| `T` | Waiting time in seconds |

The vehicle count receives a weight of 5, while the waiting time increases continuously.

---

## 3. Example

Consider two roads:

```text
Road A:
1 vehicle
50 seconds waiting

Road B:
8 vehicles
10 seconds waiting
```

The calculated priorities are:

```text
Road A:

P = 5 × 1 + 50
P = 55


Road B:

P = 5 × 8 + 10
P = 50
```

Although Road B contains more vehicles, Road A receives priority because its queue has been waiting longer.

This prevents the system from optimizing only for vehicle quantity.

---

## 4. Waiting Time Priority

The waiting time also defines qualitative priority levels.

| Waiting Time | Priority |
|---:|---|
| 0–19 seconds | Low |
| 20–39 seconds | Medium |
| 40–59 seconds | High |
| 60+ seconds | Mandatory |

At 60 seconds, the queue receives mandatory priority regardless of the competing queue size.

This mechanism prevents indefinite waiting.

---

## 5. Green Signal

The green signal does not operate exclusively according to a fixed duration.

The active road remains open while its queue is still considered significant.

When the queue becomes sufficiently small, the system evaluates the current traffic conditions again.

A simplified decision flow is:

```text
Current Road is Green
        |
        v
Is the queue still significant?
        |
     +--+--+
     |     |
    YES    NO
     |     |
     |     v
     |  Yellow
     |     |
     |     v
     | Recalculate
     |   priority
     |     |
     +-----+
           |
           v
     Select next road
```

---

## 6. Mandatory Priority

If a queue reaches 60 seconds of waiting time:

```text
Waiting Time >= 60s
        |
        v
Mandatory Priority
        |
        v
Road must be served
```

This rule takes precedence over the normal comparison of queue weights.

---

## 7. Pedestrian Request

Pedestrian requests have priority over the normal vehicle cycle.

When the pedestrian button is pressed:

```text
Pedestrian Button
        |
        v
Wait transition period
        |
        v
Yellow transition
        |
        v
Vehicle signals -> Red
        |
        v
Pedestrian Green
        |
        +----> Buzzer ON
        |
        v
Wait crossing time
        |
        v
Buzzer OFF
        |
        v
Pedestrian Green OFF
        |
        v
Pedestrian Red ON
        |
        v
Recalculate Queue Priority
        |
        v
Resume Adaptive Control
```

The prototype uses a 10-second transition delay and a 10-second pedestrian crossing period. 

> This crossing model wasn't improved due to the limited time available for prototype construction in the course.

---

## 8. Complete Decision Flow

```text
START
  |
  v
Initialize cameras
Initialize ESP-NOW
Initialize traffic lights
Initialize pedestrian LEDs
Initialize pedestrian button
Initialize buzzer
  |
  v
Receive vehicle count for Road A
Receive vehicle count for Road B
  |
  v
Update queue waiting times
  |
  v
Calculate queue priorities
  |
  v
Pedestrian button pressed?
  |
  +------------------ YES ------------------+
  |                                         |
  NO                                        v
  |                                  Wait transition
  |                                         |
  |                                         v
  |                                  Yellow transition
  |                                         |
  |                                         v
  |                                  Vehicle signals OFF
  |                                         |
  |                                         v
  |                                  Pedestrian Green
  |                                         |
  |                                         v
  |                                      Buzzer ON
  |                                         |
  |                                         v
  |                                  Wait crossing time
  |                                         |
  |                                         v
  |                                  Buzzer OFF
  |                                         |
  |                                         v
  |                                  Pedestrian Red
  |                                         |
  |                                         v
  |                                  Recalculate
  |                                         |
  +-----------------------------------------+
  |
  v
Does any queue have >= 60s waiting?
  |
  +---- YES ----> Mandatory priority
  |
  NO
  |
  v
Compare queue priorities
  |
  v
Select highest priority
  |
  v
Activate green signal
  |
  v
Is queue still significant?
  |
  +---- YES ----> Continue green
  |
  NO
  |
  v
Yellow transition
  |
  v
Recalculate priorities
  |
  v
Select next road
  |
  v
Repeat
```

---

## 9. Prototype Timing

The timing values used in this project were adapted for the academic prototype.

They were intentionally reduced because the complete system had to be demonstrated within a short presentation period.

These values should therefore not be interpreted as realistic timings for a real-world traffic signal.

In a real deployment, the parameters would need to consider:

- Traffic volume.
- Road geometry.
- Pedestrian demand.
- Local traffic regulations.
- Safety requirements.
- Intersection size.
- Peak-hour behavior.

The timing parameters are implemented in the ESP32 firmware and can be adjusted without changing the overall control architecture.

---

## 10. Design Goal

The main design principle is:

> Prioritize traffic demand while preventing excessive waiting time.

This is why the algorithm does not simply select the road with the largest number of vehicles.

Instead, it combines:

```text
Demand
  +
Fairness
  +
Waiting Time
```

into a single adaptive decision process.