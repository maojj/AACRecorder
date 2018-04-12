namespace AACRecorder.Audio
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.IO;
    using System.Runtime.CompilerServices;
    using AacEncoder;
    using CSCore;
    using CSCore.SoundIn;
    using CSCore.Streams;

    public class Recorder
    {
        private const int DefaultSampleRate = 16000;

        private const int DefaultBitRate = 64000;

        private const int DefaultChannels = 1;

        private const int BitPerSample = 16;

        private readonly int _sampleRate;

        private readonly int _bitRate;

        private readonly int _channels;

        private WaveInDevice _currentWaveInDevice;

        private WaveIn _waveIn;

        private IWaveSource _finalSource;

        private bool _encoderNeedDispose;

        private bool _canStop;

        public delegate void PeakMeterHandler(float value);

        public event PeakMeterHandler OnPeakMeter;

        public Recorder(
            int sampleRate = DefaultSampleRate,
            int bitRate = DefaultBitRate,
            int channels = DefaultChannels)
        {
            _sampleRate = sampleRate;
            _bitRate = bitRate;
            _channels = channels;

            _encoderNeedDispose = false;
        }

        ~Recorder()
        {
            if (_encoderNeedDispose)
            {
                _encoderNeedDispose = false;
                aac_enc.EncoderRelease();
            }
        }

        public bool Initialize()
        {
            _currentWaveInDevice = WaveInDevice.DefaultDevice;
            _waveIn = new WaveIn(new WaveFormat(_sampleRate, BitPerSample, _channels))
            {
                Device = _currentWaveInDevice
            };
            _waveIn.Initialize();

            if (_encoderNeedDispose)
            {
                _encoderNeedDispose = false;
                aac_enc.EncoderRelease();
            }

            if (!aac_enc.EncoderInit(_sampleRate, _channels, _bitRate))
            {
                return false;
            }

            _encoderNeedDispose = true;

            return true;
        }

        [MethodImpl(MethodImplOptions.Synchronized)]
        [SuppressMessage("Microsoft.Reliability", "CA2002:Do not lock on objects with weak identity",
                Justification = "The stream is from network, and we guarantee to lock this stream well to prevent deadlock")
        ]
        public bool StartCapture(Stream stream)
        {
            return StartCapture(
                data =>
                {
                    lock (stream)
                    {
                        stream.Write(data, 0, data.Length);
                    }
                });
        }

        public bool StartCapture(Action<byte[]> data)
        {
            var inputBufferSize = aac_enc.GetInputBufferSize();
            var outputBufferMaxSize = aac_enc.GetMaxOutputBufferSize();

            if (inputBufferSize <= 0 || outputBufferMaxSize <= 0)
            {
                return false;
            }

            var inputBuffer = new byte[inputBufferSize];
            var outputBuffer = new byte[outputBufferMaxSize];
            var outputPtr = InteropHelper.AllocByteArray(outputBuffer);

            var soundInSource = new SoundInSource(_waveIn);

            var peakMeter = new PeakMeter(soundInSource.ToSampleSource());
            _finalSource = peakMeter.ToWaveSource(BitPerSample);

            soundInSource.DataAvailable += (s, e) =>
            {
                try
                {
                    int read;
                    while ((read = _finalSource.Read(inputBuffer, 0, inputBufferSize)) > 0)
                    {
                        var inputPtr = InteropHelper.AllocByteArray(inputBuffer);
                        var size = aac_enc.EncodeBuffer(inputPtr, read, outputPtr, outputBufferMaxSize);
                        InteropHelper.FreeObjectArray(inputPtr, typeof(byte), inputBufferSize);

                        if (size > 0)
                        {
                            byte[] result;
                            if (InteropHelper.ParsePtrToByteArray(outputPtr, size, out result))
                            {
                                data.Invoke(result);
                            }
                        }
                    }
                }
                catch (Exception ex)
                {
                }
            };

            peakMeter.PeakCalculated += PeakMeterOnPeakCalculated;

            try
            {
                _waveIn.Start();
            }
            catch (MmException ex)
            {
                peakMeter.PeakCalculated -= PeakMeterOnPeakCalculated;
                return false;
            }

            _canStop = true;
            return true;
        }

        private void PeakMeterOnPeakCalculated(object sender, PeakEventArgs peakEventArgs)
        {
            OnPeakMeter?.Invoke(peakEventArgs.PeakValue);
        }

        [MethodImpl(MethodImplOptions.Synchronized)]
        public void StopCapture()
        {
            if (!_canStop)
            {
                return;
            }

            _canStop = false;

            _finalSource?.Dispose();

            if (_waveIn != null && _waveIn.RecordingState == RecordingState.Recording)
            {
                _waveIn.Stop();
            }
        }
    }
}