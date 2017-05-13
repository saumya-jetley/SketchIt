//Standard System Inclusion
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using System.Threading;
using AForge.Video;
using AForge.Video.DirectShow;
using Emgu.CV.UI;
using Emgu.CV.Structure;
using Emgu.CV;
using Emgu.Util;
using System.Diagnostics;

namespace ImageOutline
{
    public partial class Form1 : Form
    {
        //Instance Variables -  to be accessed by any method
        private FilterInfoCollection webcam;
        private VideoCaptureDevice cam;
        private int countruns = 0;
        private int flag_camatt = 0;
        private int flag_browsed = 0;
        private Image<Bgr, Byte> image;
        private IntPtr cntrpts;
        private IntPtr grayval;
        private int contour_pts_cnt;



        [DllImport("draw_outline.dll")]
        public static extern int draw_outline(IntPtr image, int rows, int cols, ref IntPtr cntrpts, ref IntPtr grayvals);

        public Form1()
        {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            radioButton1.Checked = true;
            webcam = new FilterInfoCollection(FilterCategory.VideoInputDevice);
            foreach(FilterInfo VideoCaptureDevice in webcam)
            {
                comboBox1.Items.Add(VideoCaptureDevice.Name);
                flag_camatt = 1;
            }
            if (flag_camatt == 1)
            {
                comboBox1.SelectedIndex = 0;
                cam = new VideoCaptureDevice(webcam[comboBox1.SelectedIndex].MonikerString);
                cam.NewFrame += new NewFrameEventHandler(cam_NewFrame);
                cam.Start();
            }
            Image<Gray, Byte> blankimage = new Image<Gray, Byte>(250, 250);
            blankimage = blankimage.ThresholdBinary(new Gray(-1), new Gray(255));
            pictureBox3.Image = blankimage.ToBitmap();
        }
        
        void cam_NewFrame(object sender, NewFrameEventArgs eventArgs)
        {
            if (cam.IsRunning)
            {
                Bitmap bit = (Bitmap)eventArgs.Frame.Clone();
                Bitmap resized_bit = Resize_image(bit, 250, 250);
                pictureBox1.Image = resized_bit;
            }
        }
        
        private void takepic_Click(object sender, EventArgs e)
        {
            if (cam != null && cam.IsRunning)
            {
                cam.Stop();
                cam = null;
            }
            else
            {
                if (flag_camatt == 1)
                {
                    cam = new VideoCaptureDevice(webcam[comboBox1.SelectedIndex].MonikerString);
                    cam.NewFrame += new NewFrameEventHandler(cam_NewFrame);
                    cam.Start();
                }
            }
            
            //pictureBox2.Image = pictureBox1.Image;
            /*
            saveFileDialog1.InitialDirectory = @"D:\visual_studio_test_codes\DrawYou";
            if (saveFileDialog1.ShowDialog() == DialogResult.OK)
            {
                pictureBox1.Image.Save(saveFileDialog1.FileNames[0]);
            }
            cam.NewFrame += new NewFrameEventHandler(cam_NewFrame);
            cam.Start();
            */
        }
        
        private void browse_Click(object sender, EventArgs e)
        {
            openFileDialog1.InitialDirectory = @"C:\Users";
            if (openFileDialog1.ShowDialog() == DialogResult.OK)
            {
                flag_browsed = 1;
                Bitmap inputimage = new Bitmap(openFileDialog1.FileName);
                Bitmap resizedimage = Resize_image(inputimage,250,250);
                pictureBox2.Image = resizedimage;
            }
        }

        private void comboBox1_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (cam != null && cam.IsRunning)
            {
                cam.Stop();
                cam = null;
            }
            pictureBox1.Image = null;
            cam = new VideoCaptureDevice(webcam[comboBox1.SelectedIndex].MonikerString);
            cam.NewFrame += new NewFrameEventHandler(cam_NewFrame);
            cam.Start();
        }
        
        private void radioButton2_CheckedChanged(object sender, EventArgs e)
        {
            if(radioButton2.Checked)
                radioButton1.Checked = false;
        }


        private void radioButton1_CheckedChanged(object sender, EventArgs e)
        {
            if (radioButton1.Checked)
                radioButton2.Checked = false;
        }


        private void button1_Click(object sender, EventArgs e)
        {
            //disable the sketch button
            button1.Enabled = false;
            //start processing 
            if (countruns != 0)
            {
                Image<Gray, Byte> blankimage = new Image<Gray, Byte>(250, 250);
                blankimage = blankimage.ThresholdBinary(new Gray(-1), new Gray(255));

                pictureBox3.Image.Dispose();
                pictureBox3.Image = blankimage.ToBitmap();
                pictureBox4.Image.Dispose();
                pictureBox4.Image = blankimage.ToBitmap();

                pictureBox3.Refresh();
                pictureBox4.Refresh();
            }

            if (cam != null && cam.IsRunning)
            {
                cam.Stop();
                cam = null;
            }

            if (radioButton1.Checked && flag_camatt != 1 || radioButton2.Checked && flag_browsed != 1)
            {
                MessageBox.Show("No Image Available!");
            }
            else
            {
                Bitmap orig_img;
                if (radioButton1.Checked)
                {
                    orig_img = new Bitmap(pictureBox1.Image);
                }
                else
                {
                    orig_img = new Bitmap(pictureBox2.Image);
                }

                int finalwidth = orig_img.Width;
                int finalheight = orig_img.Height;

                image = new Image<Bgr, Byte>(orig_img);
                cntrpts = new IntPtr();
                grayval = new IntPtr();

                //Start the progress bar
                
                pBar1.Style = ProgressBarStyle.Marquee;
                pBar1.MarqueeAnimationSpeed = 100000;
                
                /*
                pBar1.Style = ProgressBarStyle.Blocks;
                pBar1.Minimum = 0;
                pBar1.Maximum = 100000;
                pBar1.Step = 1;
                */
                
                pBar1.Visible = true;
                
                //main sketch processing - another thread for core processing
                Thread main_proc = new Thread(sketch_processing, 1024 * 512);
                main_proc.Start();
                //main_proc.Join();
                int count = 0;
                while (main_proc.IsAlive)
                {
                    
                    count++;
                    if(count%2000 == 0)
                      pBar1.Refresh();
                    
                    //pBar1.PerformStep();
                }
                pBar1.Visible = false;
                
                //sketch_processing();
                int[] cntrpts_arr = new int[contour_pts_cnt];
                if (cntrpts != IntPtr.Zero)
                {
                    Marshal.Copy(cntrpts, cntrpts_arr, 0, contour_pts_cnt);
                }

                int[] grayval_arr = new int[(int)contour_pts_cnt / 2];
                if (grayval != IntPtr.Zero)
                {
                    Marshal.Copy(grayval, grayval_arr, 0, (int)contour_pts_cnt / 2);
                }
                //Put it in the image 2 coordinates at a time

                Image<Gray, Byte> cntr_image = new Image<Gray, Byte>(finalwidth, finalheight);
                Image<Gray, Byte> pen_image = new Image<Gray, Byte>(finalwidth, finalheight);

                cntr_image = cntr_image.ThresholdBinary(new Gray(-1), new Gray(255));
                pen_image = pen_image.ThresholdBinary(new Gray(-1), new Gray(255));

                pictureBox4.Parent = pictureBox3;
                pictureBox4.BackColor = Color.Transparent;
                pictureBox3.BackColor = Color.Transparent;
                pictureBox4.Location = new Point(0, 0);

                pictureBox3.Image = cntr_image.ToBitmap();
                //Bitmap cats = new Bitmap("C:\\Users\\Saumya\\Desktop\\cats.jpg");
                //pictureBox3.Image = cats;

                Bitmap penimage = new Bitmap(pen_image.Width, pen_image.Height);
                penimage = pen_image.ToBitmap();
                penimage.MakeTransparent(Color.White);
                pictureBox4.Image = penimage;

                pictureBox3.Refresh();
                pictureBox4.Refresh();

                byte b = 0;
                for (int i = 0; i < contour_pts_cnt - 16; i = i + 16)
                //for (int i = contour_pts_cnt-16; i>0; i=i-16)
                {
                    b = (byte)grayval_arr[i / 2];
                    cntr_image.Data[cntrpts_arr[i + 1], cntrpts_arr[i], 0] = b;
                    b = (byte)grayval_arr[(i + 2) / 2];
                    cntr_image.Data[cntrpts_arr[i + 3], cntrpts_arr[i + 2], 0] = b;
                    b = (byte)grayval_arr[(i + 4) / 2];
                    cntr_image.Data[cntrpts_arr[i + 5], cntrpts_arr[i + 4], 0] = b;
                    b = (byte)grayval_arr[(i + 6) / 2];
                    cntr_image.Data[cntrpts_arr[i + 7], cntrpts_arr[i + 6], 0] = b;
                    b = (byte)grayval_arr[(i + 8) / 2];
                    cntr_image.Data[cntrpts_arr[i + 9], cntrpts_arr[i + 8], 0] = b;
                    b = (byte)grayval_arr[(i + 10) / 2];
                    cntr_image.Data[cntrpts_arr[i + 11], cntrpts_arr[i + 10], 0] = b;
                    b = (byte)grayval_arr[(i + 12) / 2];
                    cntr_image.Data[cntrpts_arr[i + 13], cntrpts_arr[i + 12], 0] = b;
                    b = (byte)grayval_arr[(i + 14) / 2];
                    cntr_image.Data[cntrpts_arr[i + 15], cntrpts_arr[i + 14], 0] = b;

                    //blotch
                    if (i % 16 == 0)
                    {
                        b = 0;
                        pen_image = pen_image.ThresholdBinary(new Gray(-1), new Gray(255));
                        pen_image.Data[cntrpts_arr[i + 15], cntrpts_arr[i + 14], 0] = b;
                        if ((cntrpts_arr[i + 15] - 1) >= 0 && (cntrpts_arr[i + 14] - 1) >= 0)
                        {
                            pen_image.Data[cntrpts_arr[i + 15] - 1, cntrpts_arr[i + 14], 0] = b;
                            pen_image.Data[cntrpts_arr[i + 15], cntrpts_arr[i + 14] - 1, 0] = b;
                            pen_image.Data[cntrpts_arr[i + 15] - 1, cntrpts_arr[i + 14] - 1, 0] = b;
                        }
                    }

                    /*
                    if(i!=0)
                    {
                        b = 255;
                        pen_image.Data[cntrpts_arr[i-1], cntrpts_arr[i - 2], 0] = b;
                        if ((cntrpts_arr[i -1] - 1) >= 0 && (cntrpts_arr[i -2] - 1) >= 0)
                        {
                            pen_image.Data[cntrpts_arr[i - 1] - 1, cntrpts_arr[i - 2], 0] = b;
                            pen_image.Data[cntrpts_arr[i - 1], cntrpts_arr[i - 2] - 1, 0] = b;
                            pen_image.Data[cntrpts_arr[i - 1] - 1, cntrpts_arr[i - 2] - 1, 0] = b;
                        }
              
                    }
                    */

                    //  }

                    //end-blotch

                    /*
                    b = (byte)grayval_arr[(i+4) / 2];
                    cntr_image.Data[cntrpts_arr[i+5], cntrpts_arr[i+4], 0] = b;
                    b = (byte)grayval_arr[(i+6) / 2];
                    cntr_image.Data[cntrpts_arr[i+7], cntrpts_arr[i+6], 0] = b;
                    */

                    pictureBox3.Image.Dispose();
                    pictureBox3.Image = cntr_image.ToBitmap();

                    pictureBox4.Image.Dispose();
                    penimage = pen_image.ToBitmap();
                    penimage.MakeTransparent(Color.White);
                    pictureBox4.Image = penimage;

                    pictureBox3.Refresh();
                    pictureBox4.Refresh();

                    Thread.Sleep(1);
                }
                countruns++;
            }
            button1.Enabled = true;
           
        }

        private void sketch_processing()
        {
            int colWidth = image.Cols * 3;
            int img_size = image.Rows * colWidth;
            IntPtr unmanaged = Marshal.AllocHGlobal(img_size);
            //Copy img data into unmanaged memory
            for (int i = 0; i < image.Rows; i++)
                Marshal.Copy(image.Bytes, i * image.MIplImage.WidthStep, unmanaged + i * colWidth,
                             colWidth);
            contour_pts_cnt = draw_outline(unmanaged, image.Rows, image.Cols, ref cntrpts, ref grayval);
        }

        private Bitmap Resize_image(Bitmap image, int newWidth, int newHeight)
        {
            // Get the image's original width and height
            int originalWidth = image.Width;
            int originalHeight = image.Height;

            // To preserve the aspect ratio
            float ratioX = (float)newWidth / (float)originalWidth;
            float ratioY = (float)newHeight / (float)originalHeight;
            float ratio = Math.Min(ratioX, ratioY);

            // New width and height based on aspect ratio
            int finalwidth = (int)(originalWidth * ratio);
            int finalheight = (int)(originalHeight * ratio);

            // Convert other formats (including CMYK) to RGB.
            Bitmap newImage = new Bitmap(finalwidth, finalheight, PixelFormat.Format24bppRgb);

            // Draws the image in the specified size with quality mode set to HighQuality
            using (Graphics graphics = Graphics.FromImage(newImage))
            {
                graphics.CompositingQuality = CompositingQuality.HighQuality;
                graphics.InterpolationMode = InterpolationMode.HighQualityBicubic;
                graphics.SmoothingMode = SmoothingMode.HighQuality;
                graphics.DrawImage(image, 0, 0, finalwidth, finalheight);
            }
            return (newImage);
        }

        private void button2_Click(object sender, EventArgs e)
        {
            saveFileDialog1.InitialDirectory = @"C:\Users";
            saveFileDialog1.Filter = "*.jpg|*.jpg|*.bmp|*.bmp|*.png|*.png";
            if (saveFileDialog1.ShowDialog() == DialogResult.OK && saveFileDialog1.FileName != null)
            {
                pictureBox3.Image.Save(saveFileDialog1.FileName);
            }
        }

        private void Form1_Closing(object sender, FormClosingEventArgs e)
        {
            if (cam != null && cam.IsRunning)
            {
                cam.Stop();
                cam = null;
            }

        }

        private void Form1_Closed(object sender, FormClosedEventArgs e)
        {
            this.Dispose();
            this.Close();
            Process[] processthis = Process.GetProcessesByName("ImageOutline");
            foreach (var process in processthis)
            {
                process.Kill();
            }
        }
    }
}
