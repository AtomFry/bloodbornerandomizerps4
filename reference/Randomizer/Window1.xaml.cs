using System;
using System.Collections.Generic;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

namespace MSB_Test
{
    /// <summary>
    /// Interaction logic for Window1.xaml
    /// </summary>
    public partial class Window1 : Window
    {
        public string huntersDreamIndex;
        public string hemwickIndex;
        public string cathedralWardIndex;
        public string upperIndex;
        public string mensisIndex;
        public string yahargulIndex;
        public string frontierIndex;
        public string researchIndex;
        public string oldIndex;
        public string centralIndex;
        public string cainhurstIndex;
        public string woodsIndex;
        public string byrgenwerthIndex;
        public string huntersNightmareIndex;
        public string hamletIndex;

        public int dreamScale;
        public int hemwickScale;
        public int cathedralScale;
        public int upperScale;
        public int mensisScale;
        public int yahargulScale;
        public int frontierScale;
        public int researchScale;
        public int oldScale;
        public int centralScale;
        public int cainhurstScale;
        public int woodsScale;
        public int byrgenwerthScale;
        public int nightmareScale;
        public int hamletScale;

        public Window1()
        {
            InitializeComponent();
        }


        private void HuntersDreamBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            huntersDreamIndex = HuntersDreamBox.Text;
        }

        private void CloseScalingWindowButton_Click(object sender, RoutedEventArgs e)
        {
            //public string huntersDreamIndex;
            
            
            //var window = (MainWindow)Application.Current.MainWindow;

            ////public int dreamScale;
            //window.dreamScale = dreamScale;
            ////public int hemwickScale;
            //window.hemwickScale = hemwickScale;
            ////public int cathedralScale;
            //window.cathedralScale = cathedralScale;
            ////public int upperScale;
            //window.upperScale = upperScale;
            ////public int mensisScale;
            //window.mensisScale = mensisScale;
            ////public int yahargulScale;
            //window.yahargulScale = yahargulScale;
            ////public int frontierScale;
            //window.frontierScale = frontierScale;
            ////public int researchScale;
            //window.researchScale = researchScale;
            ////public int oldScale;
            //window.oldScale = oldScale;
            ////public int centralScale;
            //window.centralScale = centralScale;
            ////public int cainhurstScale;
            //window.cainhurstScale = cainhurstScale;
            ////public int woodsScale;
            //window.woodsScale = woodsScale;
            ////public int byrgenwerthScale;
            //window.byrgenwerthScale = byrgenwerthScale;
            ////public int nightmareScale;
            //window.nightmareScale = nightmareScale;
            ////public int hamletScale;
            //window.hamletScale = hamletScale;

            Hide();
    }

        private void CathedralBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            cathedralWardIndex = CathedralBox.Text;
        }

        private void UpperBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            upperIndex = UpperBox.Text;
        }

        private void MensisBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            mensisIndex = MensisBox.Text;
        }

        private void YahargulBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            yahargulIndex = YahargulBox.Text;
        }

        private void FrontierBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            frontierIndex = FrontierBox.Text;
        }

        private void ResearchBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            researchIndex = ResearchBox.Text;
        }

        private void OldBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            oldIndex = OldBox.Text;
        }

        private void CentralBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            centralIndex = CentralBox.Text;
        }

        private void CainhurstBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            cainhurstIndex = CainhurstBox.Text;
        }

        private void WoodsBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            woodsIndex = WoodsBox.Text;
        }

        private void ByrgenwerthBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            byrgenwerthIndex = ByrgenwerthBox.Text;
        }

        private void HuntersNightmareBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            huntersNightmareIndex = HuntersNightmareBox.Text;
        }

        private void HemwickBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            hemwickIndex = HemwickBox.Text;
        }

        private void HamletBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            hamletIndex = HamletBox.Text;
        }
    }
}
