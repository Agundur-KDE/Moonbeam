These keys are synthetic test fixtures only. They have never been used
by a real Sunshine installation or for production credentials.

They exist purely to test `SunshineController`'s certificate pinning
(`real.pem`/`real.key`: a stand-in for a legitimate Sunshine certificate;
`imposter.pem`/`imposter.key`: same Common Name, different key, to prove
pinning rejects it anyway; `othertrusted.pem`/`othertrusted.key`: a
certificate deliberately added to the test process's global trust store,
to prove pinning isn't merely reactive to TLS errors). See
`tests/sunshinecontrollertest.cpp` and `tests/sunshinepinningtest.cpp`.
