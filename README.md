# Gray-Scott model with Open MPI

This repository is showcasing implementation of Gray-Scott pattern and how calculation performance improves with Open MPI.

## Performance testing

Time measured in ms, averages of 5 runs:

| n    | Sequential (ms) | Parallel 3 processes (ms) | Parallel 8 processes (ms) |
|------|-----------------|---------------------------|----------------------------|
| 256  | 757.8           | 266.0                     | 130.0                      |
| 512  | 3013.4          | 1077.6                    | 538.8                      |
| 1024 | 12115.0         | 4105.2                    | 2444.2                     |
| 2048 | 49168.2         | 16667.0                   | 11337.6                    |
