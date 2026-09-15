using System;
using System.ComponentModel;
using System.Text.RegularExpressions;
using System.Threading;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace MSB_Test
{
    public partial class MainWindow
    {
        private void BossCheckBox_Checked(object sender, RoutedEventArgs e)
        {
            includeBosses = BossCheckBox.IsChecked.Value;
            chaliceBosses = BossCheckBox.IsChecked.Value;
        }

        private void InsertBosses_Checked(object sender, RoutedEventArgs e)
        {
            insertBossesBool = InsertBosses.IsChecked.Value;
            MessageBox.Show("This may make the game unstable\r\nand cause crashing.\r\nUse this feature at your own risk.");
        }

        private void ChaliceBoss_Checked(object sender, RoutedEventArgs e)
        {
            //chaliceBosses = ChaliceBoss.IsChecked.Value;
        }

        private void OopsAllCheck_Checked(object sender, RoutedEventArgs e)
        {
            oopsAll = OopsAllCheck.IsChecked.Value;

            if (OopsAllCheck.IsChecked.Value == true)
            {
                ExEnemyBox.IsChecked = false;
                ExBossBox.IsChecked = false;
            }
        }

        private void SeedBox_TextChanged(object sender, TextChangedEventArgs e)
        {

        }

        private void OopsAllStringName_TextChanged(object sender, TextChangedEventArgs e)
        {
            oopsAllString = OopsAllStringName.Text;
        }

        private void ItemRandoCheck_Checked(object sender, RoutedEventArgs e)
        {

        }

        private void KeyItemCheck_Checked(object sender, RoutedEventArgs e)
        {

        }

        private void GetItemLot_Click(object sender, RoutedEventArgs e)
        {

        }

        private void VFXCheck_Checked(object sender, RoutedEventArgs e)
        {

        }

        private void TalkCheck_Checked(object sender, RoutedEventArgs e)
        {

        }

        private void SpEffectCheck_Checked(object sender, RoutedEventArgs e)
        {

        }

        private void BulletCheck_Checked(object sender, RoutedEventArgs e)
        {

        }

        private void GemEffectCheck_Checked(object sender, RoutedEventArgs e)
        {

        }

        private void MagicCheck_Checked(object sender, RoutedEventArgs e)
        {

        }

        private void OopsBoss_TextChanged(object sender, TextChangedEventArgs e)
        {
            oopsBossString = OopsBoss.Text;
        }

        private void OopsAllBossesCheck_Checked(object sender, RoutedEventArgs e)
        {
            oopsAllBosses = OopsAllBossesCheck.IsChecked.Value;
            includeBosses = OopsAllBossesCheck.IsChecked.Value;

            if (OopsAllBossesCheck.IsChecked.Value == true)
            {
                ExEnemyBox.IsChecked = false;
                ExBossBox.IsChecked = false;
            }
        }

        private void RandomizeEnemiesCheck_Checked(object sender, RoutedEventArgs e)
        {
            randomizeEnemiesBool = RandomizeEnemiesCheck.IsChecked.Value;
        }

        private void AddNPCS_Checked(object sender, RoutedEventArgs e)
        {
            includeNPCs = AddNPCS.IsChecked.Value;
        }

        private void ArmorRandomizerCheckBox_Checked(object sender, RoutedEventArgs e)
        {
            randomizeItemLots = ArmorRandomizerCheckBox.IsChecked.Value;
        }

        private void TotalProgressBar_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {

        }

        private void UpdateUI()
        {
            CurrentTaskLabel.Content = currentTask; ;
            TotalProgressBar.Value = totalPercent;
        }

        public void backgroundWorker_DoWork(object sender, DoWorkEventArgs e)
        {
            while (running)
            {
                UpdateUI();
            }
        }

        private void TEST_Click(object sender, RoutedEventArgs e)
        {
            // Try to pull the seed from the textbox.  If it doesn't work, then use a random seed.
            var seedString = SeedTextbox.Text.Length >= 10 ? SeedTextbox.Text.Substring(0, 10) : SeedTextbox.Text;
            if (!int.TryParse(seedString, out seed))
            {
                seed = new Random().Next();
                SeedTextbox.Text = seed.ToString();
            }
            new Thread(DoSomething).Start();
            new Thread(DoSomething2).Start();
        }

        private void BellMaidenBox_Checked(object sender, RoutedEventArgs e)
        {
            bellMaidenBool = BellMaidenBox.IsChecked.Value;
        }

        private void LesserBossesBox_Checked(object sender, RoutedEventArgs e)
        {
            lesserBossesBool = LesserBossesBox.IsChecked.Value;
        }

        private void RandomizeKeyItemsBox_Checked(object sender, RoutedEventArgs e)
        {
            keyItemRandomizeBool = RandomizeKeyItemsBox.IsChecked.Value;
        }

        private void RandomizeShopBox_Checked(object sender, RoutedEventArgs e)
        {
            shopBool = RandomizeShopBox.IsChecked.Value;
        }

        private void EnemyDropBox_Checked(object sender, RoutedEventArgs e)
        {
            enemyDropBool = EnemyDropBox.IsChecked.Value;
        }

        private void WorkshopBox_Checked(object sender, RoutedEventArgs e)
        {
            workshopBool = WorkshopBox.IsChecked.Value;
        }

        /////////////////////////////////////////////////////////////////////////////////////

        private void BossUpPointOne_Click(object sender, RoutedEventArgs e)
        {
            if (bossPercentage < 100)
            {
                bossPercentage += .1f;

                if (bossPercentage > 100)
                {
                    bossPercentage = 100;
                }

                BossPercentageLabel.Content = String.Format("{0:0.00}", bossPercentage);
            }
            else
            {
                bossPercentage = 100;
                BossPercentageLabel.Content = String.Format("{0:0.00}", bossPercentage);
            }
        }

        private void BossUpOne_Click(object sender, RoutedEventArgs e)
        {
            if (bossPercentage < 100)
            {
                bossPercentage += 1f;

                if (bossPercentage > 100)
                {
                    bossPercentage = 100;
                }

                BossPercentageLabel.Content = String.Format("{0:0.00}", bossPercentage);
            }
            else
            {
                bossPercentage = 100;
                BossPercentageLabel.Content = String.Format("{0:0.00}", bossPercentage);
            }
        }

        private void BossUpPointOne_Copy_Click(object sender, RoutedEventArgs e)
        {
            if (bossPercentage >= .1f)
            {
                bossPercentage -= .1f;

                if (bossPercentage < 0)
                {
                    bossPercentage = 0;
                }

                BossPercentageLabel.Content = String.Format("{0:0.00}", bossPercentage);
            }
            else
            {
                bossPercentage = 0;
                BossPercentageLabel.Content = String.Format("{0:0.00}", bossPercentage);
            }
        }

        private void BossUpOne_Copy_Click(object sender, RoutedEventArgs e)
        {
            if (bossPercentage >= 1)
            {
                bossPercentage -= 1f;

                if (bossPercentage < 0)
                {
                    bossPercentage = 0;
                }

                BossPercentageLabel.Content = String.Format("{0:0.00}", bossPercentage);
            }
            else
            {
                bossPercentage = 0;
                BossPercentageLabel.Content = String.Format("{0:0.00}", bossPercentage);
            }
        }


        private void BossUpTen_Click(object sender, RoutedEventArgs e)
        {
            if (bossPercentage < 100)
            {
                bossPercentage += 10f;

                if (bossPercentage > 100)
                {
                    bossPercentage = 100;
                }

                BossPercentageLabel.Content = String.Format("{0:0.00}", bossPercentage);
            }
            else
            {
                bossPercentage = 100;
                BossPercentageLabel.Content = String.Format("{0:0.00}", bossPercentage);
            }
        }

        private void BossDownTen_Click(object sender, RoutedEventArgs e)
        {
            if (bossPercentage >= 10)
            {
                bossPercentage -= 10f;

                if (bossPercentage < 0)
                {
                    bossPercentage = 0;
                }

                BossPercentageLabel.Content = String.Format("{0:0.00}", bossPercentage);
            }
            else
            {
                bossPercentage = 0;
                BossPercentageLabel.Content = String.Format("{0:0.00}", bossPercentage);
            }
        }

        /////////////////////////////////////////////////////////////////////////////////////

        private void ChaliceUpPointOne_Click(object sender, RoutedEventArgs e)
        {
            if (chaliceChanceFloat < 100)
            {
                chaliceChanceFloat += .1f;

                if (chaliceChanceFloat > 100)
                {
                    chaliceChanceFloat = 100;
                }

                ChalicePercentLabel.Content = String.Format("{0:0.00}", chaliceChanceFloat);
            }
            else
            {
                chaliceChanceFloat = 100;
                ChalicePercentLabel.Content = String.Format("{0:0.00}", chaliceChanceFloat);
            }
        }

        private void ChaliceUpOne_Click(object sender, RoutedEventArgs e)
        {
            if (chaliceChanceFloat < 100)
            {
                chaliceChanceFloat += 1f;

                if (chaliceChanceFloat > 100)
                {
                    chaliceChanceFloat = 100;
                }

                ChalicePercentLabel.Content = String.Format("{0:0.00}", chaliceChanceFloat);
            }
            else
            {
                chaliceChanceFloat = 100;
                ChalicePercentLabel.Content = String.Format("{0:0.00}", chaliceChanceFloat);
            }
        }

        private void ChaliceUpPointOne_Copy_Click(object sender, RoutedEventArgs e)
        {
            if (chaliceChanceFloat >= .1f)
            {
                chaliceChanceFloat -= .1f;

                if (chaliceChanceFloat < 0)
                {
                    chaliceChanceFloat = 0;
                }

                ChalicePercentLabel.Content = String.Format("{0:0.00}", chaliceChanceFloat);
            }
            else
            {
                chaliceChanceFloat = 0;
                ChalicePercentLabel.Content = String.Format("{0:0.00}", chaliceChanceFloat);
            }
        }

        private void ChaliceUpOne_Copy_Click(object sender, RoutedEventArgs e)
        {
            if (chaliceChanceFloat >= 1)
            {
                chaliceChanceFloat -= 1f;

                if (chaliceChanceFloat < 0)
                {
                    chaliceChanceFloat = 0;
                }

                ChalicePercentLabel.Content = String.Format("{0:0.00}", chaliceChanceFloat);
            }
            else
            {
                chaliceChanceFloat = 0;
                ChalicePercentLabel.Content = String.Format("{0:0.00}", chaliceChanceFloat);
            }
        }

        private void ChaliceUpTen_Click(object sender, RoutedEventArgs e)
        {
            if (chaliceChanceFloat < 100)
            {
                chaliceChanceFloat += 10f;

                if (chaliceChanceFloat > 100)
                {
                    chaliceChanceFloat = 100;
                }

                ChalicePercentLabel.Content = String.Format("{0:0.00}", chaliceChanceFloat);
            }
            else
            {
                chaliceChanceFloat = 100;
                ChalicePercentLabel.Content = String.Format("{0:0.00}", chaliceChanceFloat);
            }
        }

        private void ChaliceDownTen_Click(object sender, RoutedEventArgs e)
        {
            if (chaliceChanceFloat >= 10)
            {
                chaliceChanceFloat -= 10f;

                if (chaliceChanceFloat < 0)
                {
                    chaliceChanceFloat = 0;
                }

                ChalicePercentLabel.Content = String.Format("{0:0.00}", chaliceChanceFloat);
            }
            else
            {
                chaliceChanceFloat = 0;
                ChalicePercentLabel.Content = String.Format("{0:0.00}", chaliceChanceFloat);
            }
        }

        /////////////////////////////////////////////////////////////////////////////////////

        private void HCUPO_Click(object sender, RoutedEventArgs e)
        {
            if (HemwickChance < 100)
            {
                HemwickChance += .1f;

                if (HemwickChance > 100)
                {
                    HemwickChance = 100;
                }

                HemwickLabel.Content = String.Format("{0:0.00}", HemwickChance);
            }
            else
            {
                HemwickChance = 100;
                HemwickLabel.Content = String.Format("{0:0.00}", HemwickChance);
            }
        }

        private void HCUO_Click(object sender, RoutedEventArgs e)
        {
            if (HemwickChance < 100)
            {
                HemwickChance += 1f;

                if (HemwickChance > 100)
                {
                    HemwickChance = 100;
                }

                HemwickLabel.Content = String.Format("{0:0.00}", HemwickChance);
            }
            else
            {
                HemwickChance = 100;
                HemwickLabel.Content = String.Format("{0:0.00}", HemwickChance);
            }
        }

        private void HCDPO_Click(object sender, RoutedEventArgs e)
        {
            if (HemwickChance >= .1f)
            {
                HemwickChance -= .1f;

                if (HemwickChance < 0)
                {
                    HemwickChance = 0;
                }

                HemwickLabel.Content = String.Format("{0:0.00}", HemwickChance);
            }
            else
            {
                HemwickChance = 0;
                HemwickLabel.Content = String.Format("{0:0.00}", HemwickChance);
            }
        }

        private void HCDO_Click(object sender, RoutedEventArgs e)
        {
            if (HemwickChance >= 1)
            {
                HemwickChance -= 1f;

                if (HemwickChance < 0)
                {
                    HemwickChance = 0;
                }

                HemwickLabel.Content = String.Format("{0:0.00}", HemwickChance);
            }
            else
            {
                HemwickChance = 0;
                HemwickLabel.Content = String.Format("{0:0.00}", HemwickChance);
            }
        }

        private void HCUT_Click(object sender, RoutedEventArgs e)
        {
            if (HemwickChance < 100)
            {
                HemwickChance += 10f;

                if (HemwickChance > 100)
                {
                    HemwickChance = 100;
                }

                HemwickLabel.Content = String.Format("{0:0.00}", HemwickChance);
            }
            else
            {
                HemwickChance = 100;
                HemwickLabel.Content = String.Format("{0:0.00}", HemwickChance);
            }
        }

        private void HCDT_Click(object sender, RoutedEventArgs e)
        {
            if (HemwickChance >= 10)
            {
                HemwickChance -= 10f;

                if (HemwickChance < 0)
                {
                    HemwickChance = 0;
                }

                HemwickLabel.Content = String.Format("{0:0.00}", HemwickChance);
            }
            else
            {
                HemwickChance = 0;
                HemwickLabel.Content = String.Format("{0:0.00}", HemwickChance);
            }
        }

        /////////////////////////////////////////////////////////////////////////////////////

        private void CWUPO_Click(object sender, RoutedEventArgs e)
        {
            if (CathedralWardChance < 100)
            {
                CathedralWardChance += .1f;

                if (CathedralWardChance > 100)
                {
                    CathedralWardChance = 100;
                }

                CathedralWardLabel.Content = String.Format("{0:0.00}", CathedralWardChance);
            }
            else
            {
                CathedralWardChance = 100;
                CathedralWardLabel.Content = String.Format("{0:0.00}", CathedralWardChance);
            }
        }

        private void CWUO_Click(object sender, RoutedEventArgs e)
        {
            if (CathedralWardChance < 100)
            {
                CathedralWardChance += 1f;

                if (CathedralWardChance > 100)
                {
                    CathedralWardChance = 100;
                }

                CathedralWardLabel.Content = String.Format("{0:0.00}", CathedralWardChance);
            }
            else
            {
                CathedralWardChance = 100;
                CathedralWardLabel.Content = String.Format("{0:0.00}", CathedralWardChance);
            }
        }

        private void CWDPO_Click(object sender, RoutedEventArgs e)
        {
            if (CathedralWardChance >= .1f)
            {
                CathedralWardChance -= .1f;

                if (CathedralWardChance < 0)
                {
                    CathedralWardChance = 0;
                }

                CathedralWardLabel.Content = String.Format("{0:0.00}", CathedralWardChance);
            }
            else
            {
                CathedralWardChance = 0;
                CathedralWardLabel.Content = String.Format("{0:0.00}", CathedralWardChance);
            }
        }

        private void CWDO_Click(object sender, RoutedEventArgs e)
        {
            if (CathedralWardChance >= 1)
            {
                CathedralWardChance -= 1f;

                if (CathedralWardChance < 0)
                {
                    CathedralWardChance = 0;
                }

                CathedralWardLabel.Content = String.Format("{0:0.00}", CathedralWardChance);
            }
            else
            {
                CathedralWardChance = 0;
                CathedralWardLabel.Content = String.Format("{0:0.00}", CathedralWardChance);
            }
        }

        private void CWUT_Click(object sender, RoutedEventArgs e)
        {
            if (CathedralWardChance < 100)
            {
                CathedralWardChance += 10f;

                if (CathedralWardChance > 100)
                {
                    CathedralWardChance = 100;
                }

                CathedralWardLabel.Content = String.Format("{0:0.00}", CathedralWardChance);
            }
            else
            {
                CathedralWardChance = 100;
                CathedralWardLabel.Content = String.Format("{0:0.00}", CathedralWardChance);
            }
        }

        private void CWDT_Click(object sender, RoutedEventArgs e)
        {
            if (CathedralWardChance >= 10)
            {
                CathedralWardChance -= 10f;

                if (CathedralWardChance < 0)
                {
                    CathedralWardChance = 0;
                }

                CathedralWardLabel.Content = String.Format("{0:0.00}", CathedralWardChance);
            }
            else
            {
                CathedralWardChance = 0;
                CathedralWardLabel.Content = String.Format("{0:0.00}", CathedralWardChance);
            }
        }

        /////////////////////////////////////////////////////////////////////////////////////

        private void UCUPO_Click(object sender, RoutedEventArgs e)
        {
            if (UpperCathedralWardChance < 100)
            {
                UpperCathedralWardChance += .1f;

                if (UpperCathedralWardChance > 100)
                {
                    UpperCathedralWardChance = 100;
                }

                UpperCathedralLabel.Content = String.Format("{0:0.00}", UpperCathedralWardChance);
            }
            else
            {
                UpperCathedralWardChance = 100;
                UpperCathedralLabel.Content = String.Format("{0:0.00}", UpperCathedralWardChance);
            }
        }

        private void UCUO_Click(object sender, RoutedEventArgs e)
        {
            if (UpperCathedralWardChance < 100)
            {
                UpperCathedralWardChance += 1f;

                if (UpperCathedralWardChance > 100)
                {
                    UpperCathedralWardChance = 100;
                }

                UpperCathedralLabel.Content = String.Format("{0:0.00}", UpperCathedralWardChance);
            }
            else
            {
                UpperCathedralWardChance = 100;
                UpperCathedralLabel.Content = String.Format("{0:0.00}", UpperCathedralWardChance);
            }
        }

        private void UCDPO_Click(object sender, RoutedEventArgs e)
        {
            if (UpperCathedralWardChance >= .1f)
            {
                UpperCathedralWardChance -= .1f;

                if (UpperCathedralWardChance < 0)
                {
                    UpperCathedralWardChance = 0;
                }

                UpperCathedralLabel.Content = String.Format("{0:0.00}", UpperCathedralWardChance);
            }
            else
            {
                UpperCathedralWardChance = 0;
                UpperCathedralLabel.Content = String.Format("{0:0.00}", UpperCathedralWardChance);
            }
        }

        private void UCDO_Click(object sender, RoutedEventArgs e)
        {
            if (UpperCathedralWardChance >= 1)
            {
                UpperCathedralWardChance -= 1f;

                if (UpperCathedralWardChance < 0)
                {
                    UpperCathedralWardChance = 0;
                }

                UpperCathedralLabel.Content = String.Format("{0:0.00}", UpperCathedralWardChance);
            }
            else
            {
                UpperCathedralWardChance = 0;
                UpperCathedralLabel.Content = String.Format("{0:0.00}", UpperCathedralWardChance);
            }
        }

        private void UCUT_Click(object sender, RoutedEventArgs e)
        {
            if (UpperCathedralWardChance < 100)
            {
                UpperCathedralWardChance += 10f;

                if (UpperCathedralWardChance > 100)
                {
                    UpperCathedralWardChance = 100;
                }

                UpperCathedralLabel.Content = String.Format("{0:0.00}", UpperCathedralWardChance);
            }
            else
            {
                UpperCathedralWardChance = 100;
                UpperCathedralLabel.Content = String.Format("{0:0.00}", UpperCathedralWardChance);
            }
        }

        private void UCDT_Click(object sender, RoutedEventArgs e)
        {
            if (UpperCathedralWardChance >= 10)
            {
                UpperCathedralWardChance -= 10f;

                if (UpperCathedralWardChance < 0)
                {
                    UpperCathedralWardChance = 0;
                }

                UpperCathedralLabel.Content = String.Format("{0:0.00}", UpperCathedralWardChance);
            }
            else
            {
                UpperCathedralWardChance = 0;
                UpperCathedralLabel.Content = String.Format("{0:0.00}", UpperCathedralWardChance);
            }
        }

        /////////////////////////////////////////////////////////////////////////////////////

        private void MUPO_Click(object sender, RoutedEventArgs e)
        {
            if (NightmareOfMensisChance < 100)
            {
                NightmareOfMensisChance += .1f;

                if (NightmareOfMensisChance > 100)
                {
                    NightmareOfMensisChance = 100;
                }

                MensisLabel.Content = String.Format("{0:0.00}", NightmareOfMensisChance);
            }
            else
            {
                NightmareOfMensisChance = 100;
                MensisLabel.Content = String.Format("{0:0.00}", NightmareOfMensisChance);
            }
        }

        private void MUO_Click(object sender, RoutedEventArgs e)
        {
            if (NightmareOfMensisChance < 100)
            {
                NightmareOfMensisChance += 1f;

                if (NightmareOfMensisChance > 100)
                {
                    NightmareOfMensisChance = 100;
                }

                MensisLabel.Content = String.Format("{0:0.00}", NightmareOfMensisChance);
            }
            else
            {
                NightmareOfMensisChance = 100;
                MensisLabel.Content = String.Format("{0:0.00}", NightmareOfMensisChance);
            }
        }

        private void MDPO_Click(object sender, RoutedEventArgs e)
        {
            if (NightmareOfMensisChance >= .1f)
            {
                NightmareOfMensisChance -= .1f;

                if (NightmareOfMensisChance < 0)
                {
                    NightmareOfMensisChance = 0;
                }

                MensisLabel.Content = String.Format("{0:0.00}", NightmareOfMensisChance);
            }
            else
            {
                NightmareOfMensisChance = 0;
                MensisLabel.Content = String.Format("{0:0.00}", NightmareOfMensisChance);
            }
        }

        private void MDO_Click(object sender, RoutedEventArgs e)
        {
            if (NightmareOfMensisChance >= 1)
            {
                NightmareOfMensisChance -= 1f;

                if (NightmareOfMensisChance < 0)
                {
                    NightmareOfMensisChance = 0;
                }

                MensisLabel.Content = String.Format("{0:0.00}", NightmareOfMensisChance);
            }
            else
            {
                NightmareOfMensisChance = 0;
                MensisLabel.Content = String.Format("{0:0.00}", NightmareOfMensisChance);
            }
        }

        private void MUT_Click(object sender, RoutedEventArgs e)
        {
            if (NightmareOfMensisChance < 100)
            {
                NightmareOfMensisChance += 10f;

                if (NightmareOfMensisChance > 100)
                {
                    NightmareOfMensisChance = 100;
                }

                MensisLabel.Content = String.Format("{0:0.00}", NightmareOfMensisChance);
            }
            else
            {
                NightmareOfMensisChance = 100;
                MensisLabel.Content = String.Format("{0:0.00}", NightmareOfMensisChance);
            }
        }

        private void MDT_Click(object sender, RoutedEventArgs e)
        {
            if (NightmareOfMensisChance >= 10)
            {
                NightmareOfMensisChance -= 10f;

                if (NightmareOfMensisChance < 0)
                {
                    NightmareOfMensisChance = 0;
                }

                MensisLabel.Content = String.Format("{0:0.00}", NightmareOfMensisChance);
            }
            else
            {
                NightmareOfMensisChance = 0;
                MensisLabel.Content = String.Format("{0:0.00}", NightmareOfMensisChance);
            }
        }

        /////////////////////////////////////////////////////////////////////////////////////

        private void YUPO_Click(object sender, RoutedEventArgs e)
        {
            if (YahargulChance < 100)
            {
                YahargulChance += .1f;

                if (YahargulChance > 100)
                {
                    YahargulChance = 100;
                }

                YahargulLabel.Content = String.Format("{0:0.00}", YahargulChance);
            }
            else
            {
                YahargulChance = 100;
                YahargulLabel.Content = String.Format("{0:0.00}", YahargulChance);
            }
        }

        private void YUO_Click(object sender, RoutedEventArgs e)
        {
            if (YahargulChance < 100)
            {
                YahargulChance += 1f;

                if (YahargulChance > 100)
                {
                    YahargulChance = 100;
                }

                YahargulLabel.Content = String.Format("{0:0.00}", YahargulChance);
            }
            else
            {
                YahargulChance = 100;
                YahargulLabel.Content = String.Format("{0:0.00}", YahargulChance);
            }
        }

        private void YDPO_Click(object sender, RoutedEventArgs e)
        {
            if (YahargulChance >= .1f)
            {
                YahargulChance -= .1f;

                if (YahargulChance < 0)
                {
                    YahargulChance = 0;
                }

                YahargulLabel.Content = String.Format("{0:0.00}", YahargulChance);
            }
            else
            {
                YahargulChance = 0;
                YahargulLabel.Content = String.Format("{0:0.00}", YahargulChance);
            }
        }

        private void YDO_Click(object sender, RoutedEventArgs e)
        {
            if (YahargulChance >= 1)
            {
                YahargulChance -= 1f;

                if (YahargulChance < 0)
                {
                    YahargulChance = 0;
                }

                YahargulLabel.Content = String.Format("{0:0.00}", YahargulChance);
            }
            else
            {
                YahargulChance = 0;
                YahargulLabel.Content = String.Format("{0:0.00}", YahargulChance);
            }
        }

        private void YUT_Click(object sender, RoutedEventArgs e)
        {
            if (YahargulChance < 100)
            {
                YahargulChance += 10f;

                if (YahargulChance > 100)
                {
                    YahargulChance = 100;
                }

                YahargulLabel.Content = String.Format("{0:0.00}", YahargulChance);
            }
            else
            {
                YahargulChance = 100;
                YahargulLabel.Content = String.Format("{0:0.00}", YahargulChance);
            }
        }

        private void YDT_Click(object sender, RoutedEventArgs e)
        {
            if (YahargulChance >= 10)
            {
                YahargulChance -= 10f;

                if (YahargulChance < 0)
                {
                    YahargulChance = 0;
                }

                YahargulLabel.Content = String.Format("{0:0.00}", YahargulChance);
            }
            else
            {
                YahargulChance = 0;
                YahargulLabel.Content = String.Format("{0:0.00}", YahargulChance);
            }
        }

        /////////////////////////////////////////////////////////////////////////////////////

        private void FUPO_Click(object sender, RoutedEventArgs e)
        {
            if (FrontierChance < 100)
            {
                FrontierChance += .1f;

                if (FrontierChance > 100)
                {
                    FrontierChance = 100;
                }

                FrontierLabel.Content = String.Format("{0:0.00}", FrontierChance);
            }
            else
            {
                FrontierChance = 100;
                FrontierLabel.Content = String.Format("{0:0.00}", FrontierChance);
            }
        }

        private void FUO_Click(object sender, RoutedEventArgs e)
        {
            if (FrontierChance < 100)
            {
                FrontierChance += 1f;

                if (FrontierChance > 100)
                {
                    FrontierChance = 100;
                }

                FrontierLabel.Content = String.Format("{0:0.00}", FrontierChance);
            }
            else
            {
                FrontierChance = 100;
                FrontierLabel.Content = String.Format("{0:0.00}", FrontierChance);
            }
        }

        private void FDPO_Click(object sender, RoutedEventArgs e)
        {
            if (FrontierChance >= .1f)
            {
                FrontierChance -= .1f;

                if (FrontierChance < 0)
                {
                    FrontierChance = 0;
                }

                FrontierLabel.Content = String.Format("{0:0.00}", FrontierChance);
            }
            else
            {
                FrontierChance = 0;
                FrontierLabel.Content = String.Format("{0:0.00}", FrontierChance);
            }
        }

        private void FDO_Click(object sender, RoutedEventArgs e)
        {
            if (FrontierChance >= 1)
            {
                FrontierChance -= 1f;

                if (FrontierChance < 0)
                {
                    FrontierChance = 0;
                }

                FrontierLabel.Content = String.Format("{0:0.00}", FrontierChance);
            }
            else
            {
                FrontierChance = 0;
                FrontierLabel.Content = String.Format("{0:0.00}", FrontierChance);
            }
        }

        private void FUT_Click(object sender, RoutedEventArgs e)
        {
            if (FrontierChance < 100)
            {
                FrontierChance += 10f;

                if (FrontierChance > 100)
                {
                    FrontierChance = 100;
                }

                FrontierLabel.Content = String.Format("{0:0.00}", FrontierChance);
            }
            else
            {
                FrontierChance = 100;
                FrontierLabel.Content = String.Format("{0:0.00}", FrontierChance);
            }
        }

        private void FDT_Click(object sender, RoutedEventArgs e)
        {
            if (FrontierChance >= 10)
            {
                FrontierChance -= 10f;

                if (FrontierChance < 0)
                {
                    FrontierChance = 0;
                }

                FrontierLabel.Content = String.Format("{0:0.00}", FrontierChance);
            }
            else
            {
                FrontierChance = 0;
                FrontierLabel.Content = String.Format("{0:0.00}", FrontierChance);
            }
        }

        /////////////////////////////////////////////////////////////////////////////////////

        private void RUPO_Click(object sender, RoutedEventArgs e)
        {
            if (ResearchHallChance < 100)
            {
                ResearchHallChance += .1f;

                if (ResearchHallChance > 100)
                {
                    ResearchHallChance = 100;
                }

                ResearchLabel.Content = String.Format("{0:0.00}", ResearchHallChance);
            }
            else
            {
                ResearchHallChance = 100;
                ResearchLabel.Content = String.Format("{0:0.00}", ResearchHallChance);
            }
        }

        private void RUO_Click(object sender, RoutedEventArgs e)
        {
            if (ResearchHallChance < 100)
            {
                ResearchHallChance += 1f;

                if (ResearchHallChance > 100)
                {
                    ResearchHallChance = 100;
                }

                ResearchLabel.Content = String.Format("{0:0.00}", ResearchHallChance);
            }
            else
            {
                ResearchHallChance = 100;
                ResearchLabel.Content = String.Format("{0:0.00}", ResearchHallChance);
            }
        }

        private void RDPO_Click(object sender, RoutedEventArgs e)
        {
            if (ResearchHallChance >= .1f)
            {
                ResearchHallChance -= .1f;

                if (ResearchHallChance < 0)
                {
                    ResearchHallChance = 0;
                }

                ResearchLabel.Content = String.Format("{0:0.00}", ResearchHallChance);
            }
            else
            {
                ResearchHallChance = 0;
                ResearchLabel.Content = String.Format("{0:0.00}", ResearchHallChance);
            }
        }

        private void RDO_Click(object sender, RoutedEventArgs e)
        {
            if (ResearchHallChance >= 1)
            {
                ResearchHallChance -= 1f;

                if (ResearchHallChance < 0)
                {
                    ResearchHallChance = 0;
                }

                ResearchLabel.Content = String.Format("{0:0.00}", ResearchHallChance);
            }
            else
            {
                ResearchHallChance = 0;
                ResearchLabel.Content = String.Format("{0:0.00}", ResearchHallChance);
            }
        }

        private void RUT_Click(object sender, RoutedEventArgs e)
        {
            if (ResearchHallChance < 100)
            {
                ResearchHallChance += 10f;

                if (ResearchHallChance > 100)
                {
                    ResearchHallChance = 100;
                }

                ResearchLabel.Content = String.Format("{0:0.00}", ResearchHallChance);
            }
            else
            {
                ResearchHallChance = 100;
                ResearchLabel.Content = String.Format("{0:0.00}", FrontierChance);
            }
        }

        private void RDT_Click(object sender, RoutedEventArgs e)
        {
            if (ResearchHallChance >= 10)
            {
                ResearchHallChance -= 10f;

                if (ResearchHallChance < 0)
                {
                    ResearchHallChance = 0;
                }

                ResearchLabel.Content = String.Format("{0:0.00}", ResearchHallChance);
            }
            else
            {
                ResearchHallChance = 0;
                ResearchLabel.Content = String.Format("{0:0.00}", ResearchHallChance);
            }
        }

        /////////////////////////////////////////////////////////////////////////////////////

        private void OYUPO_Click(object sender, RoutedEventArgs e)
        {
            if (OldYharnamChance < 100)
            {
                OldYharnamChance += .1f;

                if (OldYharnamChance > 100)
                {
                    OldYharnamChance = 100;
                }

                OldYharnamLabel.Content = String.Format("{0:0.00}", OldYharnamChance);
            }
            else
            {
                OldYharnamChance = 100;
                OldYharnamLabel.Content = String.Format("{0:0.00}", OldYharnamChance);
            }
        }

        private void OYUO_Click(object sender, RoutedEventArgs e)
        {
            if (OldYharnamChance < 100)
            {
                OldYharnamChance += 1f;

                if (OldYharnamChance > 100)
                {
                    OldYharnamChance = 100;
                }

                OldYharnamLabel.Content = String.Format("{0:0.00}", OldYharnamChance);
            }
            else
            {
                OldYharnamChance = 100;
                OldYharnamLabel.Content = String.Format("{0:0.00}", OldYharnamChance);
            }
        }

        private void OYDPO_Click(object sender, RoutedEventArgs e)
        {
            if (OldYharnamChance >= .1f)
            {
                OldYharnamChance -= .1f;

                if (OldYharnamChance < 0)
                {
                    OldYharnamChance = 0;
                }

                OldYharnamLabel.Content = String.Format("{0:0.00}", OldYharnamChance);
            }
            else
            {
                OldYharnamChance = 0;
                OldYharnamLabel.Content = String.Format("{0:0.00}", OldYharnamChance);
            }
        }

        private void OYDO_Click(object sender, RoutedEventArgs e)
        {
            if (OldYharnamChance >= 1)
            {
                OldYharnamChance -= 1f;

                if (OldYharnamChance < 0)
                {
                    OldYharnamChance = 0;
                }

                OldYharnamLabel.Content = String.Format("{0:0.00}", OldYharnamChance);
            }
            else
            {
                OldYharnamChance = 0;
                OldYharnamLabel.Content = String.Format("{0:0.00}", OldYharnamChance);
            }
        }

        private void OYUT_Click(object sender, RoutedEventArgs e)
        {
            if (OldYharnamChance < 100)
            {
                OldYharnamChance += 10f;

                if (OldYharnamChance > 100)
                {
                    OldYharnamChance = 100;
                }

                OldYharnamLabel.Content = String.Format("{0:0.00}", OldYharnamChance);
            }
            else
            {
                OldYharnamChance = 100;
                OldYharnamLabel.Content = String.Format("{0:0.00}", FrontierChance);
            }
        }

        private void OYDT_Click(object sender, RoutedEventArgs e)
        {
            if (OldYharnamChance >= 10)
            {
                OldYharnamChance -= 10f;

                if (OldYharnamChance < 0)
                {
                    OldYharnamChance = 0;
                }

                OldYharnamLabel.Content = String.Format("{0:0.00}", OldYharnamChance);
            }
            else
            {
                OldYharnamChance = 0;
                OldYharnamLabel.Content = String.Format("{0:0.00}", OldYharnamChance);
            }
        }

        /////////////////////////////////////////////////////////////////////////////////////

        private void CYUPO_Click(object sender, RoutedEventArgs e)
        {
            if (CentralYharnamChance < 100)
            {
                CentralYharnamChance += .1f;

                if (CentralYharnamChance > 100)
                {
                    CentralYharnamChance = 100;
                }

                CentralYharnamLabel.Content = String.Format("{0:0.00}", CentralYharnamChance);
            }
            else
            {
                CentralYharnamChance = 100;
                CentralYharnamLabel.Content = String.Format("{0:0.00}", CentralYharnamChance);
            }
        }

        private void CYUO_Click(object sender, RoutedEventArgs e)
        {
            if (CentralYharnamChance < 100)
            {
                CentralYharnamChance += 1f;

                if (CentralYharnamChance > 100)
                {
                    CentralYharnamChance = 100;
                }

                CentralYharnamLabel.Content = String.Format("{0:0.00}", CentralYharnamChance);
            }
            else
            {
                CentralYharnamChance = 100;
                CentralYharnamLabel.Content = String.Format("{0:0.00}", CentralYharnamChance);
            }
        }

        private void CYDPO_Click(object sender, RoutedEventArgs e)
        {
            if (CentralYharnamChance >= .1f)
            {
                CentralYharnamChance -= .1f;

                if (CentralYharnamChance < 0)
                {
                    CentralYharnamChance = 0;
                }

                CentralYharnamLabel.Content = String.Format("{0:0.00}", CentralYharnamChance);
            }
            else
            {
                CentralYharnamChance = 0;
                CentralYharnamLabel.Content = String.Format("{0:0.00}", CentralYharnamChance);
            }
        }

        private void CYDO_Click(object sender, RoutedEventArgs e)
        {
            if (CentralYharnamChance >= 1)
            {
                CentralYharnamChance -= 1f;

                if (CentralYharnamChance < 0)
                {
                    CentralYharnamChance = 0;
                }

                CentralYharnamLabel.Content = String.Format("{0:0.00}", CentralYharnamChance);
            }
            else
            {
                CentralYharnamChance = 0;
                CentralYharnamLabel.Content = String.Format("{0:0.00}", CentralYharnamChance);
            }
        }

        private void CYUT_Click(object sender, RoutedEventArgs e)
        {
            if (CentralYharnamChance < 100)
            {
                CentralYharnamChance += 10f;

                if (CentralYharnamChance > 100)
                {
                    CentralYharnamChance = 100;
                }

                CentralYharnamLabel.Content = String.Format("{0:0.00}", CentralYharnamChance);
            }
            else
            {
                CentralYharnamChance = 100;
                CentralYharnamLabel.Content = String.Format("{0:0.00}", FrontierChance);
            }
        }

        private void CYDT_Click(object sender, RoutedEventArgs e)
        {
            if (CentralYharnamChance >= 10)
            {
                CentralYharnamChance -= 10f;

                if (CentralYharnamChance < 0)
                {
                    CentralYharnamChance = 0;
                }

                CentralYharnamLabel.Content = String.Format("{0:0.00}", CentralYharnamChance);
            }
            else
            {
                CentralYharnamChance = 0;
                CentralYharnamLabel.Content = String.Format("{0:0.00}", CentralYharnamChance);
            }
        }

        /////////////////////////////////////////////////////////////////////////////////////

        private void CUPO_Click(object sender, RoutedEventArgs e)
        {
            if (CainhurstChance < 100)
            {
                CainhurstChance += .1f;

                if (CainhurstChance > 100)
                {
                    CainhurstChance = 100;
                }

                CainhurstLabel.Content = String.Format("{0:0.00}", CainhurstChance);
            }
            else
            {
                CainhurstChance = 100;
                CainhurstLabel.Content = String.Format("{0:0.00}", CainhurstChance);
            }
        }

        private void CUO_Click(object sender, RoutedEventArgs e)
        {
            if (CainhurstChance < 100)
            {
                CainhurstChance += 1f;

                if (CainhurstChance > 100)
                {
                    CainhurstChance = 100;
                }

                CainhurstLabel.Content = String.Format("{0:0.00}", CainhurstChance);
            }
            else
            {
                CainhurstChance = 100;
                CainhurstLabel.Content = String.Format("{0:0.00}", CainhurstChance);
            }
        }

        private void CDPO_Click(object sender, RoutedEventArgs e)
        {
            if (CainhurstChance >= .1f)
            {
                CainhurstChance -= .1f;

                if (CainhurstChance < 0)
                {
                    CainhurstChance = 0;
                }

                CainhurstLabel.Content = String.Format("{0:0.00}", CainhurstChance);
            }
            else
            {
                CainhurstChance = 0;
                CainhurstLabel.Content = String.Format("{0:0.00}", CainhurstChance);
            }
        }

        private void CDO_Click(object sender, RoutedEventArgs e)
        {
            if (CainhurstChance >= 1)
            {
                CainhurstChance -= 1f;

                if (CainhurstChance < 0)
                {
                    CainhurstChance = 0;
                }

                CainhurstLabel.Content = String.Format("{0:0.00}", CainhurstChance);
            }
            else
            {
                CainhurstChance = 0;
                CainhurstLabel.Content = String.Format("{0:0.00}", CainhurstChance);
            }
        }

        private void CUT_Click(object sender, RoutedEventArgs e)
        {
            if (CainhurstChance < 100)
            {
                CainhurstChance += 10f;

                if (CainhurstChance > 100)
                {
                    CainhurstChance = 100;
                }

                CainhurstLabel.Content = String.Format("{0:0.00}", CainhurstChance);
            }
            else
            {
                CainhurstChance = 100;
                CainhurstLabel.Content = String.Format("{0:0.00}", FrontierChance);
            }
        }

        private void CDT_Click(object sender, RoutedEventArgs e)
        {
            if (CainhurstChance >= 10)
            {
                CainhurstChance -= 10f;

                if (CainhurstChance < 0)
                {
                    CainhurstChance = 0;
                }

                CainhurstLabel.Content = String.Format("{0:0.00}", CainhurstChance);
            }
            else
            {
                CainhurstChance = 0;
                CainhurstLabel.Content = String.Format("{0:0.00}", CainhurstChance);
            }
        }

        /////////////////////////////////////////////////////////////////////////////////////

        private void WUPO_Click(object sender, RoutedEventArgs e)
        {
            if (WoodsChance < 100)
            {
                WoodsChance += .1f;

                if (WoodsChance > 100)
                {
                    WoodsChance = 100;
                }

                WoodsLabel.Content = String.Format("{0:0.00}", WoodsChance);
            }
            else
            {
                WoodsChance = 100;
                WoodsLabel.Content = String.Format("{0:0.00}", WoodsChance);
            }
        }

        private void WUO_Click(object sender, RoutedEventArgs e)
        {
            if (WoodsChance < 100)
            {
                WoodsChance += 1f;

                if (WoodsChance > 100)
                {
                    WoodsChance = 100;
                }

                WoodsLabel.Content = String.Format("{0:0.00}", WoodsChance);
            }
            else
            {
                WoodsChance = 100;
                WoodsLabel.Content = String.Format("{0:0.00}", WoodsChance);
            }
        }

        private void WDPO_Click(object sender, RoutedEventArgs e)
        {
            if (WoodsChance >= .1f)
            {
                WoodsChance -= .1f;

                if (WoodsChance < 0)
                {
                    WoodsChance = 0;
                }

                WoodsLabel.Content = String.Format("{0:0.00}", WoodsChance);
            }
            else
            {
                WoodsChance = 0;
                WoodsLabel.Content = String.Format("{0:0.00}", WoodsChance);
            }
        }

        private void WDO_Click(object sender, RoutedEventArgs e)
        {
            if (WoodsChance >= 1)
            {
                WoodsChance -= 1f;

                if (WoodsChance < 0)
                {
                    WoodsChance = 0;
                }

                WoodsLabel.Content = String.Format("{0:0.00}", WoodsChance);
            }
            else
            {
                WoodsChance = 0;
                WoodsLabel.Content = String.Format("{0:0.00}", WoodsChance);
            }
        }

        private void WUT_Click(object sender, RoutedEventArgs e)
        {
            if (WoodsChance < 100)
            {
                WoodsChance += 10f;

                if (WoodsChance > 100)
                {
                    WoodsChance = 100;
                }

                WoodsLabel.Content = String.Format("{0:0.00}", WoodsChance);
            }
            else
            {
                WoodsChance = 100;
                WoodsLabel.Content = String.Format("{0:0.00}", FrontierChance);
            }
        }

        private void WDT_Click(object sender, RoutedEventArgs e)
        {
            if (WoodsChance >= 10)
            {
                WoodsChance -= 10f;

                if (WoodsChance < 0)
                {
                    WoodsChance = 0;
                }

                WoodsLabel.Content = String.Format("{0:0.00}", WoodsChance);
            }
            else
            {
                WoodsChance = 0;
                WoodsLabel.Content = String.Format("{0:0.00}", WoodsChance);
            }
        }

        /////////////////////////////////////////////////////////////////////////////////////

        private void BUPO_Click(object sender, RoutedEventArgs e)
        {
            if (ByrgenwerthChance < 100)
            {
                ByrgenwerthChance += .1f;

                if (ByrgenwerthChance > 100)
                {
                    ByrgenwerthChance = 100;
                }

                ByrgenwerthLabel.Content = String.Format("{0:0.00}", ByrgenwerthChance);
            }
            else
            {
                ByrgenwerthChance = 100;
                ByrgenwerthLabel.Content = String.Format("{0:0.00}", ByrgenwerthChance);
            }
        }

        private void BUO_Click(object sender, RoutedEventArgs e)
        {
            if (ByrgenwerthChance < 100)
            {
                ByrgenwerthChance += 1f;

                if (ByrgenwerthChance > 100)
                {
                    ByrgenwerthChance = 100;
                }

                ByrgenwerthLabel.Content = String.Format("{0:0.00}", ByrgenwerthChance);
            }
            else
            {
                ByrgenwerthChance = 100;
                ByrgenwerthLabel.Content = String.Format("{0:0.00}", ByrgenwerthChance);
            }
        }

        private void BDPO_Click(object sender, RoutedEventArgs e)
        {
            if (ByrgenwerthChance >= .1f)
            {
                ByrgenwerthChance -= .1f;

                if (ByrgenwerthChance < 0)
                {
                    ByrgenwerthChance = 0;
                }

                ByrgenwerthLabel.Content = String.Format("{0:0.00}", ByrgenwerthChance);
            }
            else
            {
                ByrgenwerthChance = 0;
                ByrgenwerthLabel.Content = String.Format("{0:0.00}", ByrgenwerthChance);
            }
        }

        private void BDO_Click(object sender, RoutedEventArgs e)
        {
            if (ByrgenwerthChance >= 1)
            {
                ByrgenwerthChance -= 1f;

                if (ByrgenwerthChance < 0)
                {
                    ByrgenwerthChance = 0;
                }

                ByrgenwerthLabel.Content = String.Format("{0:0.00}", ByrgenwerthChance);
            }
            else
            {
                ByrgenwerthChance = 0;
                ByrgenwerthLabel.Content = String.Format("{0:0.00}", ByrgenwerthChance);
            }
        }

        private void BUT_Click(object sender, RoutedEventArgs e)
        {
            if (ByrgenwerthChance < 100)
            {
                ByrgenwerthChance += 10f;

                if (ByrgenwerthChance > 100)
                {
                    ByrgenwerthChance = 100;
                }

                ByrgenwerthLabel.Content = String.Format("{0:0.00}", ByrgenwerthChance);
            }
            else
            {
                ByrgenwerthChance = 100;
                ByrgenwerthLabel.Content = String.Format("{0:0.00}", FrontierChance);
            }
        }

        private void BDT_Click(object sender, RoutedEventArgs e)
        {
            if (ByrgenwerthChance >= 10)
            {
                ByrgenwerthChance -= 10f;

                if (ByrgenwerthChance < 0)
                {
                    ByrgenwerthChance = 0;
                }

                ByrgenwerthLabel.Content = String.Format("{0:0.00}", ByrgenwerthChance);
            }
            else
            {
                ByrgenwerthChance = 0;
                ByrgenwerthLabel.Content = String.Format("{0:0.00}", ByrgenwerthChance);
            }
        }

        /////////////////////////////////////////////////////////////////////////////////////

        private void HUPO_Click(object sender, RoutedEventArgs e)
        {
            if (HuntersNightmareChance < 100)
            {
                HuntersNightmareChance += .1f;

                if (HuntersNightmareChance > 100)
                {
                    HuntersNightmareChance = 100;
                }

                HuntersNightmareLabel.Content = String.Format("{0:0.00}", HuntersNightmareChance);
            }
            else
            {
                HuntersNightmareChance = 100;
                HuntersNightmareLabel.Content = String.Format("{0:0.00}", HuntersNightmareChance);
            }
        }

        private void HUO_Click(object sender, RoutedEventArgs e)
        {
            if (HuntersNightmareChance < 100)
            {
                HuntersNightmareChance += 1f;

                if (HuntersNightmareChance > 100)
                {
                    HuntersNightmareChance = 100;
                }

                HuntersNightmareLabel.Content = String.Format("{0:0.00}", HuntersNightmareChance);
            }
            else
            {
                HuntersNightmareChance = 100;
                HuntersNightmareLabel.Content = String.Format("{0:0.00}", HuntersNightmareChance);
            }
        }

        private void HDPO_Click(object sender, RoutedEventArgs e)
        {
            if (HuntersNightmareChance >= .1f)
            {
                HuntersNightmareChance -= .1f;

                if (HuntersNightmareChance < 0)
                {
                    HuntersNightmareChance = 0;
                }

                HuntersNightmareLabel.Content = String.Format("{0:0.00}", HuntersNightmareChance);
            }
            else
            {
                HuntersNightmareChance = 0;
                HuntersNightmareLabel.Content = String.Format("{0:0.00}", HuntersNightmareChance);
            }
        }

        private void HDO_Click(object sender, RoutedEventArgs e)
        {
            if (HuntersNightmareChance >= 1)
            {
                HuntersNightmareChance -= 1f;

                if (HuntersNightmareChance < 0)
                {
                    HuntersNightmareChance = 0;
                }

                HuntersNightmareLabel.Content = String.Format("{0:0.00}", HuntersNightmareChance);
            }
            else
            {
                HuntersNightmareChance = 0;
                HuntersNightmareLabel.Content = String.Format("{0:0.00}", HuntersNightmareChance);
            }
        }

        private void HUT_Click(object sender, RoutedEventArgs e)
        {
            if (HuntersNightmareChance < 100)
            {
                HuntersNightmareChance += 10f;

                if (HuntersNightmareChance > 100)
                {
                    HuntersNightmareChance = 100;
                }

                HuntersNightmareLabel.Content = String.Format("{0:0.00}", HuntersNightmareChance);
            }
            else
            {
                HuntersNightmareChance = 100;
                HuntersNightmareLabel.Content = String.Format("{0:0.00}", FrontierChance);
            }
        }

        private void HDT_Click(object sender, RoutedEventArgs e)
        {
            if (HuntersNightmareChance >= 10)
            {
                HuntersNightmareChance -= 10f;

                if (HuntersNightmareChance < 0)
                {
                    HuntersNightmareChance = 0;
                }

                HuntersNightmareLabel.Content = String.Format("{0:0.00}", HuntersNightmareChance);
            }
            else
            {
                HuntersNightmareChance = 0;
                HuntersNightmareLabel.Content = String.Format("{0:0.00}", HuntersNightmareChance);
            }
        }

        /////////////////////////////////////////////////////////////////////////////////////

        private void FHUPO_Click(object sender, RoutedEventArgs e)
        {
            if (HamletChance < 100)
            {
                HamletChance += .1f;

                if (HamletChance > 100)
                {
                    HamletChance = 100;
                }

                FishingHamletLabel.Content = String.Format("{0:0.00}", HamletChance);
            }
            else
            {
                HamletChance = 100;
                FishingHamletLabel.Content = String.Format("{0:0.00}", HamletChance);
            }
        }

        private void FHUO_Click(object sender, RoutedEventArgs e)
        {
            if (HamletChance < 100)
            {
                HamletChance += 1f;

                if (HamletChance > 100)
                {
                    HamletChance = 100;
                }

                FishingHamletLabel.Content = String.Format("{0:0.00}", HamletChance);
            }
            else
            {
                HamletChance = 100;
                FishingHamletLabel.Content = String.Format("{0:0.00}", HamletChance);
            }
        }

        private void FHDPO_Click(object sender, RoutedEventArgs e)
        {
            if (HamletChance >= .1f)
            {
                HamletChance -= .1f;

                if (HamletChance < 0)
                {
                    HamletChance = 0;
                }

                FishingHamletLabel.Content = String.Format("{0:0.00}", HamletChance);
            }
            else
            {
                HamletChance = 0;
                FishingHamletLabel.Content = String.Format("{0:0.00}", HamletChance);
            }
        }

        private void FHDO_Click(object sender, RoutedEventArgs e)
        {
            if (HamletChance >= 1)
            {
                HamletChance -= 1f;

                if (HamletChance < 0)
                {
                    HamletChance = 0;
                }

                FishingHamletLabel.Content = String.Format("{0:0.00}", HamletChance);
            }
            else
            {
                HamletChance = 0;
                FishingHamletLabel.Content = String.Format("{0:0.00}", HamletChance);
            }
        }

        private void FHUT_Click(object sender, RoutedEventArgs e)
        {
            if (HamletChance < 100)
            {
                HamletChance += 10f;

                if (HamletChance > 100)
                {
                    HamletChance = 100;
                }

                FishingHamletLabel.Content = String.Format("{0:0.00}", HamletChance);
            }
            else
            {
                HamletChance = 100;
                FishingHamletLabel.Content = String.Format("{0:0.00}", FrontierChance);
            }
        }

        private void FHDT_Click(object sender, RoutedEventArgs e)
        {
            if (HamletChance >= 10)
            {
                HamletChance -= 10f;

                if (HamletChance < 0)
                {
                    HamletChance = 0;
                }

                FishingHamletLabel.Content = String.Format("{0:0.00}", HamletChance);
            }
            else
            {
                HamletChance = 0;
                FishingHamletLabel.Content = String.Format("{0:0.00}", HamletChance);
            }
        }

        private void KeepGunsBox_Checked(object sender, RoutedEventArgs e)
        {
            keepGuns = KeepGunsBox.IsChecked.Value;
        }

        private void HemwickCheckBox_Checked(object sender, RoutedEventArgs e)
        {
            bossHemwick = HemwickCheckBox.IsChecked.Value;
        }

        private void CathedralWardBossBox_Checked(object sender, RoutedEventArgs e)
        {
            bossCathedralWard = CathedralWardBossBox.IsChecked.Value;
        }

        private void UpperCathedralBossBox_Checked(object sender, RoutedEventArgs e)
        {
            bossUpperCathedralWard = UpperCathedralBossBox.IsChecked.Value;
        }

        private void MensisBossBox_Checked(object sender, RoutedEventArgs e)
        {
            bossMensis = MensisBossBox.IsChecked.Value;
        }

        private void FrontierBossBox_Checked(object sender, RoutedEventArgs e)
        {
            bossFrontier = FrontierBossBox.IsChecked.Value;
        }

        private void YahargulBossBox_Checked(object sender, RoutedEventArgs e)
        {
            bossYahargul = YahargulBossBox.IsChecked.Value;
        }

        private void ResearchHallBossBox_Checked(object sender, RoutedEventArgs e)
        {
            bossResearchHall = ResearchHallBossBox.IsChecked.Value;
        }

        private void OldYharnamBossBox_Checked(object sender, RoutedEventArgs e)
        {
            bossOldYharnam = OldYharnamBossBox.IsChecked.Value;
        }

        private void CentralYharnamBossBox_Checked(object sender, RoutedEventArgs e)
        {
            bossCentralYharnam = CentralYharnamBossBox.IsChecked.Value;
        }

        private void CainhurstBossBox_Checked(object sender, RoutedEventArgs e)
        {
            bossCainhurst = CainhurstBossBox.IsChecked.Value;
        }

        private void ByrgenwerthBossBox_Checked(object sender, RoutedEventArgs e)
        {
            bossByrgenwerthLecture = ByrgenwerthBossBox.IsChecked.Value;
        }

        private void WoodsBossBox_Checked(object sender, RoutedEventArgs e)
        {
            bossForbiddenWoods = WoodsBossBox.IsChecked.Value;
        }

        private void NightmareBossBox_Checked(object sender, RoutedEventArgs e)
        {
            bossNightmare = NightmareBossBox.IsChecked.Value;
        }

        private void HamletBossBox_Checked(object sender, RoutedEventArgs e)
        {
            bossHamlet = HamletBossBox.IsChecked.Value;
        }

        private void ChaliceBossBox_Checked(object sender, RoutedEventArgs e)
        {
            bossChalices = ChaliceBossBox.IsChecked.Value;
        }

        private void EnemySizeSlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            sizeOfEnemy = EnemySizeSlider.Value;
        }

        private void StartingWeaponsRandomizeBox_Checked(object sender, RoutedEventArgs e)
        {
            startingWeaponsOnlyBool = StartingWeaponsRandomizeBox.IsChecked.Value;
        }

        private void EasyMultiBossesBox_Checked(object sender, RoutedEventArgs e)
        {
            easyMultiBossesBool = EasyMultiBossesBox.IsChecked.Value;
        }

        private void EasyFailuresBox_Checked(object sender, RoutedEventArgs e)
        {
            easyFailuresBool = EasyFailuresBox.IsChecked.Value;
        }

        private void VFXChange_Checked(object sender, RoutedEventArgs e)
        {
            vfxBool = VFXChange.IsChecked.Value;
        }

        private void FaceBox_Checked(object sender, RoutedEventArgs e)
        {
            faceBool = FaceBox.IsChecked.Value;
        }

        private void GemsAndRunesBox_Checked(object sender, RoutedEventArgs e)
        {
            gemBool = GemsAndRunesBox.IsChecked.Value;
        }

        private void TalkBox_Checked(object sender, RoutedEventArgs e)
        {
            talkBool = TalkBox.IsChecked.Value;
        }

        private void BloodBox_Checked(object sender, RoutedEventArgs e)
        {
            bloodBool = BloodBox.IsChecked.Value;
        }

        private void ExEnemyBox_Checked(object sender, RoutedEventArgs e)
        {
            excludeEnemiesBool = ExEnemyBox.IsChecked.Value;

            if (ExEnemyBox.IsChecked.Value == true)
            {
                OopsAllCheck.IsChecked = false;
                OopsAllBossesCheck.IsChecked = false;
            }
        }

        private void ExBossBox_Checked(object sender, RoutedEventArgs e)
        {
            excludeBossesBool = ExBossBox.IsChecked.Value;

            if (ExBossBox.IsChecked.Value == true)
            {
                OopsAllCheck.IsChecked = false;
                OopsAllBossesCheck.IsChecked = false;
            }
        }

        private void EasyRomBox_Checked(object sender, RoutedEventArgs e)
        {
            easyRomBool = EasyRomBox.IsChecked.Value;
        }

        private void EasyWitchesBox_Checked(object sender, RoutedEventArgs e)
        {
            easyWitchesBool = EasyWitchesBox.IsChecked.Value;
        }

        private void AllDmgAll_Checked(object sender, RoutedEventArgs e)
        {
            //cell 100 team type 25
            teamTypeBool = AllDmgAll.IsChecked.Value;
        }

        private void SixGemBox_Checked(object sender, RoutedEventArgs e)
        {
            sixGemBool = SixGemBox.IsChecked.Value;
            if (ThreeGemBox.IsChecked.Value == true)
            {
                ThreeGemBox.IsChecked = false;
            }
        }

        private void NoScaleBox_Checked(object sender, RoutedEventArgs e)
        {
            noScalingBool = NoScaleBox.IsChecked.Value;
        }

        private void PermaDarknessBox_Checked(object sender, RoutedEventArgs e)
        {
            permaDarknessBool = PermaDarknessBox.IsChecked.Value;
        }

        private void GunMovesetBox_Checked(object sender, RoutedEventArgs e)
        {
            gunMoveset = GunMovesetBox.IsChecked.Value;
        }

        private void MeleeMovesetBox_Checked(object sender, RoutedEventArgs e)
        {
            meleeMoveset = MeleeMovesetBox.IsChecked.Value;
        }

        private void ThreeGemBox_Checked(object sender, RoutedEventArgs e)
        {
            threeGemBool = ThreeGemBox.IsChecked.Value;
            if (SixGemBox.IsChecked.Value == true)
            {
                SixGemBox.IsChecked = false;
            }
        }

        private void ScaleWindowButton_Click(object sender, RoutedEventArgs e)
        {
            win1.Show();
        }

        private void CustomScalingBox_Checked(object sender, RoutedEventArgs e)
        {
            customScaling = CustomScalingBox.IsChecked.Value;
        }

        private void Chalice_Checked(object sender, RoutedEventArgs e)
        {
            chaliceEnemies = ChaliceEnemies.IsChecked.Value;
        }
        private void NumberValidationTextBox(object sender, TextCompositionEventArgs e)
        {
            Regex regex = new Regex("[^0-9]+");
            e.Handled = regex.IsMatch(e.Text);
        }


        //private void CutEnemyBox_Checked(object sender, RoutedEventArgs e)
        //{
        //    cutEnemyBool = CutEnemyBox.IsChecked.Value;
        //}



        /////////////////////////////////////////////////////////////////////////////////////

    }
}
