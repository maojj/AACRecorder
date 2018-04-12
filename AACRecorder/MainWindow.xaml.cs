
namespace AACRecorder
{
    using System.IO;
    using System.Threading;

    using AACRecorder.Audio;
    using System.Windows;

    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
            var recorder = new Recorder();
            recorder.Initialize();

            var stream = File.Create(@"D:\test.aac");
            recorder.StartCapture(stream);

            Thread.Sleep(10000);
            recorder.StopCapture();
        }
    }
}
