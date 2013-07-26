Aquire
======

python client for Neuroscan Acquire 4.3 EEG server

To use the synamp^2 amplifier with proprietry software (Acquire 4.3)
to capture EEG, EOG and ECG data (up to 64 channels + bipolar channels)
and use as input for neurofeedback software (OpenViBE).

Requires "driver"

Acquire will send control header and data packets over TCP.

Need front end for OpenViBE or alternative BCI software (Unlock Project)

Started with basic python server client using socket library

see: 'client.py'
     'server.py'



