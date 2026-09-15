using SoulsFormats;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading;
using System.Windows;

namespace MSB_Test
{
    public partial class MainWindow
    {
        private void DoSomething2()
        {
            running = true;

            while (running)
            {
                Thread.Sleep(1000);
                totalTime += 1;

                var span = new TimeSpan(0, 0, totalTime);
                var yourStr = string.Format("{0}:{1:00}", (int)span.TotalMinutes, span.Seconds);

                this.Dispatcher.Invoke(() =>
                {
                    TotalTime.Content = yourStr;
                });
            }
        }

        private void DoSomething()
        {
            universalRand = new Random(seed);

            // Will allow for consistent key item placement, even if all the other randomization parameters are different.  
            // Lets you use multiple different randomizers in a single run in case one crashes.  
            keyitemRand = new Random(seed);

            this.Dispatcher.Invoke(() =>
            {
                SetBooleans();
            });

            if (bellMaidenBool)
            {
                unusedPlusBossList.Add("c1050");
                unusedPlusBossList.Add("c1051");
                unusedPlusBossList.Add("c1055");
            }

            currentDirectory = Directory.GetCurrentDirectory();
            var parentDir = Directory.GetParent(currentDirectory);
            filePath = parentDir + "\\dvdroot_ps4";

            var logFile = File.ReadAllLines(filePath + "\\Mod Files\\NPC Scaling File\\NPCScalingFile.txt");
            logList = new List<string>(logFile);

            longList = new List<long>();
            for (int i = 0; i < logList.Count; i++)
            {
                longList.Add(long.Parse(logList[i]));
            }

            var loggFile = File.ReadAllLines(filePath + "\\Mod Files\\NPC Scaling File\\NPCParamsFile.txt");
            logList = new List<string>(loggFile);

            npcParams = new List<long>();
            for (int i = 0; i < logList.Count; i++)
            {
                npcParams.Add(long.Parse(logList[i]));
            }

            var nameFile = File.ReadAllLines(filePath + "\\Mod Files\\NPC Scaling File\\Names.txt");
            for (int i = 0; i < nameFile.Length; i++)
            {
                nameList.Add(nameFile[i]);
            }

            var sizeFile = File.ReadAllLines(filePath + "\\Mod Files\\NPC Scaling File\\Sizes.txt");
            for (int i = 0; i < sizeFile.Length; i++)
            {
                sizeList.Add(long.Parse(sizeFile[i]));
            }

            for (int i = 0; i < nameList.Count; i++)
            {
                namesAndSizes.Add(nameList[i], sizeList[i]);
            }

            mapList = new List<string>();

            this.Dispatcher.Invoke(() =>
            {
                totalPercent = 5;
                currentTask = "Backing up Files...";
                UpdateUI();
            });

            mapList.Add(filePath + "\\map\\mapstudio\\" + "m21_00_00_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m21_01_00_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m22_00_00_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m23_00_00_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m23_00_00_01.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m24_00_00_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m24_00_00_01.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m24_01_00_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m24_01_00_01.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m24_01_00_11.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m24_02_00_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m24_02_00_01.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m25_00_00_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m26_00_00_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m27_00_00_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m27_00_00_01.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m28_00_00_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m28_00_00_01.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m32_00_00_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m32_00_00_01.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m33_00_00_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m34_00_00_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m35_00_00_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m36_00_00_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_10_90_00\\m29_10_90_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_10_90_00\\m29_10_90_01.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_20_90_00\\m29_20_90_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_20_90_00\\m29_20_90_01.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_21_90_00\\m29_21_90_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_30_90_00\\m29_30_90_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_30_90_00\\m29_30_90_01.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_31_90_00\\m29_31_90_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_40_90_00\\m29_40_90_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_40_90_00\\m29_40_90_01.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_42_90_00\\m29_42_90_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_42_90_00\\m29_42_90_01.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_50_90_00\\m29_50_90_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_50_90_00\\m29_50_90_01.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_52_90_00\\m29_52_90_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_52_90_00\\m29_52_90_01.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_53_90_00\\m29_53_90_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_53_90_00\\m29_53_90_01.msb.dcx");

            if (File.Exists(filePath + "\\map\\mapstudio\\" + "m29_50_40_00\\m29_50_40_00.msb.dcx"))
            {
                mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_50_40_00\\m29_50_40_00.msb.dcx");
                GOBAdded = true;
            }

            eventFileList.Add(filePath + "\\event\\m21_00_00_00.emevd.dcx");
            eventFileList.Add(filePath + "\\event\\m21_01_00_00.emevd.dcx");
            eventFileList.Add(filePath + "\\event\\m22_00_00_00.emevd.dcx");
            eventFileList.Add(filePath + "\\event\\m23_00_00_00.emevd.dcx");
            eventFileList.Add(filePath + "\\event\\m24_00_00_00.emevd.dcx");
            eventFileList.Add(filePath + "\\event\\m24_01_00_00.emevd.dcx");
            eventFileList.Add(filePath + "\\event\\m24_02_00_00.emevd.dcx");
            eventFileList.Add(filePath + "\\event\\m25_00_00_00.emevd.dcx");
            eventFileList.Add(filePath + "\\event\\m26_00_00_00.emevd.dcx");
            eventFileList.Add(filePath + "\\event\\m27_00_00_00.emevd.dcx");
            eventFileList.Add(filePath + "\\event\\m28_00_00_00.emevd.dcx");
            eventFileList.Add(filePath + "\\event\\m32_00_00_00.emevd.dcx");
            eventFileList.Add(filePath + "\\event\\m33_00_00_00.emevd.dcx");
            eventFileList.Add(filePath + "\\event\\m34_00_00_00.emevd.dcx");
            eventFileList.Add(filePath + "\\event\\m35_00_00_00.emevd.dcx");
            eventFileList.Add(filePath + "\\event\\m36_00_00_00.emevd.dcx");

            string dateNowww = DateTime.Now.ToString("h:mm:ss tt");
            string buttsss = dateNowww.Replace(":", "-");
            sizeFilePath = filePath + "\\Mod Files\\Logs. Don't Delete\\" + buttsss + "-EnemySizes.txt";
            if (logging)
                using (FileStream sw1 = File.Create(sizeFilePath))
                {

                }

            string dateNowwww = DateTime.Now.ToString("h:mm:ss tt");
            string buttssss = dateNowwww.Replace(":", "-");
            insertedBossPath = filePath + "\\Mod Files\\Logs. Don't Delete\\" + buttssss + "-InsertedBossesLog.txt";
            if (logging)
                using (FileStream sw1 = File.Create(insertedBossPath))
            {

            }

            string dateNowwwww = DateTime.Now.ToString("h:mm:ss tt");
            string buttsssss = dateNowwwww.Replace(":", "-");
            randomizedNPCPath = filePath + "\\Mod Files\\Logs. Don't Delete\\" + buttsssss + "-RandomizedNPCLog.txt";
            if (logging)
                if (logging)
                using (FileStream sw1 = File.Create(randomizedNPCPath))
                {

                }

            string dateNowwwwww = DateTime.Now.ToString("h:mm:ss tt");
            string buttssssss = dateNowwwwww.Replace(":", "-");
            randomizedItemLotPath = filePath + "\\Mod Files\\Logs. Don't Delete\\" + buttssssss + "-RandomizedItemLog.txt";
            if (logging)
                if (logging)
                using (FileStream sw1 = File.Create(randomizedItemLotPath))
                {

                }

            for (int i = 0; i < mapList.Count; i++)
            {
                if (File.Exists(mapList[i] + ".bak"))
                {
                    var tempMapRead = MSBB.Read(mapList[i] + ".bak");

                    long maxSizeLong = 0;

                    for (int j = 0; j < tempMapRead.Parts.Enemies.Count; j++)
                    {
                        for (int k = 0; k < nameList.Count; k++)
                        {
                            if (tempMapRead.Parts.Enemies[j].ModelName.Contains(nameList[k]))
                            {
                                maxSizeLong += sizeList[k];
                                k = nameList.Count + 1;
                            }
                        }
                    }

                    //maxSizeList.Add(maxSizeLong);

                    if (logging)
                        using (StreamWriter writetext = File.AppendText(sizeFilePath))
                        {
                            //writetext.WriteLine("MAX SIZE " + mapList[i] + " " + maxSizeLong + " " + tempMapRead.Parts.Enemies.Count);
                        }
                }
            }

            paramPath = filePath + "\\param\\gameparam\\gameparam.parambnd.dcx";
            paramDefPath = filePath + "\\paramdef\\paramdef.paramdefbnd.dcx";

            if (File.Exists(paramPath + ".bak"))
            {
                File.Delete(paramPath);
                File.Copy(paramPath + ".bak", paramPath);
            }

            if (File.Exists(paramPath + ".bak"))
            {
                File.Delete(paramPath + ".bak");
            }
            File.Copy(paramPath, paramPath + ".bak");

            if (File.Exists(eventFileList[0] + ".bak"))
            {
                for (int i = 0; i < eventFileList.Count; i++)
                {
                    File.Delete(eventFileList[i]);
                }

                for (int i = 0; i < eventFileList.Count; i++)
                {
                    File.Copy(eventFileList[i] + ".bak", eventFileList[i]);
                    File.Delete(eventFileList[i] + ".bak");
                }
            }

            for (int i = 0; i < eventFileList.Count; i++)
            {
                File.Copy(eventFileList[i], eventFileList[i] + ".bak");
            }

            List<string> tempDirList = new List<string>();
            List<string> chaliceMapListString = new List<string>();

            //if(seedString == "" || seedString == "Insert Seed.... Numbers Only")


            for (int i = 0; i < mapList.Count; i++)
            {
                if (!File.Exists(mapList[i] + ".bak"))
                {
                    File.Copy(mapList[i], mapList[i] + ".bak");
                }

                else if (File.Exists(mapList[i] + ".bak"))
                {
                    File.Delete(mapList[i]);
                    File.Copy(mapList[i] + ".bak", mapList[i]);
                }
            }

            /*
            if (File.Exists(mapList[0] + ".bak"))
            {
                for (int i = 0; i < mapList.Count; i++)
                {
                    File.Delete(mapList[i]);
                }

                for (int i = 0; i < mapList.Count; i++)
                {
                    File.Copy(mapList[i] + ".bak", mapList[i]);
                    File.Delete(mapList[i] + ".bak");
                }
            }

            for (int i = 0; i < mapList.Count; i++)
            {
                File.Copy(mapList[i], mapList[i] + ".bak");
            }
            */
            string dateNo = DateTime.Now.ToString("h:mm:ss tt");
            string butt = dateNo.Replace(":", "-");
            modelFilePath = filePath + "\\Mod Files\\Logs. Don't Delete\\" + butt + "-ModelLog.txt";
            if (logging)
                using (FileStream sw = File.Create(modelFilePath))
                {

                }

            //setting models in list to paste into all maps
            for (int i = 0; i < mapList.Count; i++)
            {
                var tempAss = MSBB.Read(mapList[i]);

                for (int j = 0; j < tempAss.Models.Enemies.Count; j++)
                {
                    modelList.Add(tempAss.Models.Enemies[j]);
                }
            }

            /*
            if (cutEnemyBool)
            {
                string[] filePaths = Directory.GetFiles(filePath + "\\MAPS\\", "*.dcx", SearchOption.TopDirectoryOnly);

                List<string> nMapStringList = new List<string>();

                for (int i = 0; i < filePaths.Length; i++)
                {
                    nMapStringList.Add(filePaths[i]);
                }

                List<MSBB.Model.Enemy> cutEnemyModelList = new List<MSBB.Model.Enemy>();

                for (int i = 0; i < nMapStringList.Count; i ++)
                {
                    var tempMapp = MSBB.Read(nMapStringList[i]);

                    for(int j = 0; j < tempMapp.Models.Enemies.Count; j ++)
                    {
                        modelList.Add(tempMapp.Models.Enemies[j]);
                    }
                }
            }
            */
            var tempMap = MSBB.Read(filePath + "\\map\\mapstudio\\" + "\\m29_52_01_00\\m29_52_01_91.msb.dcx");

            for (int i = 0; i < tempMap.Models.Enemies.Count; i++)
            {
                modelList.Add(tempMap.Models.Enemies[i]);
            }

            for (int i = 0; i < modelList.Count; i++)
            {
                if (logging)
                    using (StreamWriter writetext = File.AppendText(modelFilePath))
                    {
                        //writetext.WriteLine("NAME " + modelList[i].Name);
                        //writetext.WriteLine("PLCH " + modelList[i].Placeholder);
                    }
            }

            for (int i = 0; i < mapList.Count; i++)
            {
                var tempAss = MSBB.Read(mapList[i]);

                for (int j = 0; j < modelList.Count; j++)
                {
                    if (!tempAss.Models.Enemies.Contains(modelList[j]))
                    {
                        tempAss.Models.Enemies.Add(modelList[j]);
                    }
                }

                tempAss.Write(mapList[i]);
            }

            if (oopsAll)
            {

                if (oopsAllString.Length > 5)
                {
                    string tempOopsString = oopsAllString.Replace(" ", "");
                    int totalStrings = tempOopsString.Length / 5;
                    int startingNumber = 0;
                    for (int i = 0; i < totalStrings; i++)
                    {
                        oopsAllList.Add(tempOopsString.Substring(startingNumber, 5));
                        startingNumber = startingNumber + 5;
                    }
                }
                else if (oopsAllString.Length == 5)
                {
                    oopsAllList.Add(oopsAllString);
                }

                oopsAllEnemyList = new List<MSBB.Part.Enemy>();

                for (int i = 0; i < mapList.Count; i++)
                {
                    OopsAll(mapList[i]);
                }


                for (int i = 0; i < oopsAllEnemyList.Count; i++)
                {
                    string tempString = oopsAllEnemyList[i].NPCParamID.ToString() + "*" + oopsAllEnemyList[i].ThinkParamID.ToString() + "*" + oopsAllEnemyList[i].ModelName;
                    if (oopsAllEnemyList[i].ThinkParamID.ToString() != "1")
                    {
                        oopsAllEnemyString.Add(oopsAllEnemyList[i].NPCParamID.ToString() + "*" + oopsAllEnemyList[i].ThinkParamID.ToString() + "*" + oopsAllEnemyList[i].ModelName);

                    }
                }
                if (oopsAllString.Contains("c1230"))
                {
                    oopsAllEnemyString.Add("110123000*123000*c1230");
                }
            }

            if (oopsAllBosses)
            {
                if (oopsBossString.Length > 5)
                {
                    string tempOopsString = oopsBossString.Replace(" ", "");
                    int totalStrings = tempOopsString.Length / 5;
                    int startingNumber = 0;
                    for (int i = 0; i < totalStrings; i++)
                    {
                        oopsAllBossString.Add(tempOopsString.Substring(startingNumber, 5));
                        startingNumber = startingNumber + 5;
                    }
                }
                else if (oopsBossString.Length == 5)
                {
                    oopsAllBossString.Add(oopsBossString);
                }

                oopsAllBossList = new List<MSBB.Part.Enemy>();

                for (int i = 0; i < mapList.Count; i++)
                {
                    OopsAllBoss(mapList[i]);
                }

                for (int i = 0; i < oopsAllBossList.Count; i++)
                {
                    string tempString = oopsAllBossList[i].NPCParamID.ToString() + "*" + oopsAllBossList[i].ThinkParamID.ToString() + "*" + oopsAllBossList[i].ModelName;
                    if (oopsAllBossList[i].ThinkParamID.ToString() != "1")
                    {
                        BossListString.Add(oopsAllBossList[i].NPCParamID.ToString() + "*" + oopsAllBossList[i].ThinkParamID.ToString() + "*" + oopsAllBossList[i].ModelName);

                    }
                }

                if (oopsBossString.Contains("c1230"))
                {
                    BossListString.Add("110123000*123000*c1230");
                }
            }

            if (!oopsAll)
            {
                this.Dispatcher.Invoke(() =>
                {
                    totalPercent = 15;
                    currentTask = "Generating Enemy List...";
                    UpdateUI();
                });

                GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m21_00_00_00.msb.dcx", unusedPlusBossList);
                GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m21_01_00_00.msb.dcx", unusedPlusBossList);
                GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m22_00_00_00.msb.dcx", unusedPlusBossList);
                GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m23_00_00_00.msb.dcx", unusedPlusBossList);
                GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m23_00_00_01.msb.dcx", unusedPlusBossList);
                GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m24_00_00_00.msb.dcx", unusedPlusBossList);
                GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m24_00_00_01.msb.dcx", unusedPlusBossList);
                GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m24_01_00_00.msb.dcx", unusedPlusBossList);
                GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m24_01_00_01.msb.dcx", unusedPlusBossList);
                GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m24_01_00_11.msb.dcx", unusedPlusBossList);
                GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m24_02_00_00.msb.dcx", unusedPlusBossList);
                GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m24_02_00_01.msb.dcx", unusedPlusBossList);
                GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m25_00_00_00.msb.dcx", unusedPlusBossList);
                GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m26_00_00_00.msb.dcx", unusedPlusBossList);
                GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m27_00_00_00.msb.dcx", unusedPlusBossList);
                GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m27_00_00_01.msb.dcx", unusedPlusBossList);
                GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m28_00_00_00.msb.dcx", unusedPlusBossList);
                GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m28_00_00_01.msb.dcx", unusedPlusBossList);
                GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m32_00_00_00.msb.dcx", unusedPlusBossList);
                GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m32_00_00_01.msb.dcx", unusedPlusBossList);
                GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m33_00_00_00.msb.dcx", unusedPlusBossList);
                GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m34_00_00_00.msb.dcx", unusedPlusBossList);
                GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m35_00_00_00.msb.dcx", unusedPlusBossList);
                GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m36_00_00_00.msb.dcx", unusedPlusBossList);
                if (chaliceEnemies)
                {
                    GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m29_10_90_00\\m29_10_90_00.msb.dcx", unusedPlusBossList);
                    //DoStuff(filePath + "\\map\\mapstudio\\" + "m29_10_90_00\\m29_10_90_01.msb.dcx", nonoList);
                    GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m29_20_90_00\\m29_20_90_00.msb.dcx", unusedPlusBossList);
                    //DoStuff(filePath + "\\map\\mapstudio\\" + "m29_20_90_00\\m29_20_90_01.msb.dcx", nonoList);
                    GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m29_21_90_00\\m29_21_90_00.msb.dcx", unusedPlusBossList);
                    GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m29_30_90_00\\m29_30_90_00.msb.dcx", unusedPlusBossList);
                    //DoStuff(filePath + "\\map\\mapstudio\\" + "m29_30_90_00\\m29_30_90_01.msb.dcx", nonoList);
                    GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m29_31_90_00\\m29_31_90_00.msb.dcx", unusedPlusBossList);
                    GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m29_40_90_00\\m29_40_90_00.msb.dcx", unusedPlusBossList);
                    //DoStuff(filePath + "\\map\\mapstudio\\" + "m29_40_90_00\\m29_40_90_01.msb.dcx", nonoList);
                    GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m29_42_90_00\\m29_42_90_00.msb.dcx", unusedPlusBossList);
                    //DoStuff(filePath + "\\map\\mapstudio\\" + "m29_42_90_00\\m29_42_90_01.msb.dcx", nonoList);
                    GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m29_50_90_00\\m29_50_90_00.msb.dcx", unusedPlusBossList);
                    //DoStuff(filePath + "\\map\\mapstudio\\" + "m29_50_90_00\\m29_50_90_01.msb.dcx", nonoList);
                    GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m29_52_90_00\\m29_52_90_00.msb.dcx", unusedPlusBossList);
                    //DoStuff(filePath + "\\map\\mapstudio\\" + "m29_52_90_00\\m29_52_90_01.msb.dcx", nonoList);
                    GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m29_53_90_00\\m29_53_90_00.msb.dcx", unusedPlusBossList);
                    //DoStuff(filePath + "\\map\\mapstudio\\" + "m29_53_90_00\\m29_53_90_01.msb.dcx", nonoList);
                    /*
                    GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m29_01_00_00\\m29_01_00_00.msb.dcx", unusedPlusBossList);
                    GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m29_01_00_00\\m29_01_00_33.msb.dcx", unusedPlusBossList);
                    GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m29_01_00_00\\m29_01_00_36.msb.dcx", unusedPlusBossList);
                    GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m29_01_00_00\\m29_01_00_45.msb.dcx", unusedPlusBossList);
                    GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m29_01_00_00\\m29_01_00_99.msb.dcx", unusedPlusBossList);
                    GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m29_01_02_00\\m29_01_02_03.msb.dcx", unusedPlusBossList);
                    GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m29_01_99_00\\m29_01_99_99.msb.dcx", unusedPlusBossList);
                    GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m29_09_00_00\\m29_09_00_00.msb.dcx", unusedPlusBossList);
                    */
                    if (GOBAdded)
                    {
                        GenerateEnemyList(filePath + "\\map\\mapstudio\\" + "m29_50_40_00\\m29_50_40_00.msb.dcx", unusedPlusBossList);
                    }

                    if (excludeEnemiesBool)
                    {
                        if (oopsAllString.Length > 5)
                        {
                            string tempOopsString = oopsAllString.Replace(" ", "");
                            int totalStrings = tempOopsString.Length / 5;
                            int startingNumber = 0;
                            for (int i = 0; i < totalStrings; i++)
                            {
                                oopsAllList.Add(tempOopsString.Substring(startingNumber, 5));
                                startingNumber = startingNumber + 5;
                            }
                        }
                        else if (oopsAllString.Length == 5)
                        {
                            oopsAllList.Add(oopsAllString);
                        }

                        for (int i = enemyData.Count - 1; i > -1; i--)
                        {
                            for (int j = 0; j < oopsAllList.Count; j++)
                            {
                                if (enemyData[i].Contains(oopsAllList[j]))
                                {
                                    enemyData.RemoveAt(i);
                                }
                            }
                        }
                        if (chaliceEnemies)
                        {
                            for (int i = chaliceEnemiesString.Count - 1; i > -1; i--)
                            {
                                for (int j = 0; j < oopsAllList.Count; j++)
                                {
                                    if (chaliceEnemiesString[i].Contains(oopsAllList[j]))
                                    {
                                        chaliceEnemiesString.RemoveAt(i);
                                    }
                                }
                            }
                        }
                    }
                }

                unusedPlusBossList.RemoveAt(3);

                if (randomizeEnemiesBool)
                {
                    while (enemyData.Count > 0)
                    {
                        int index = universalRand.Next(0, enemyData.Count);
                        enemyDataRandomized.Add(enemyData[index]);
                        enemyData.RemoveAt(index);
                    }

                    List<string> uniqueEnemyList = enemyDataRandomized.Distinct().ToList();
                    enemyDataRandomized = new List<string>();

                    for (int i = 0; i < uniqueEnemyList.Count; i++)
                    {
                        enemyDataRandomized.Add(uniqueEnemyList[i]);
                    }

                    //CutEnemyScan();

                    //if (cutEnemyBool)
                    //{
                    //enemyDataRandomized = new List<string>();
                    //for (int i = 0; i < cutEnemyString.Count; i++)
                    //{
                    //enemyDataRandomized.Add(cutEnemyString[i]);
                    //}
                    //}

                    if (chaliceEnemies)
                    {
                        List<string> uniqueEnemyListc = chaliceEnemiesString.Distinct().ToList();
                        chaliceEnemiesString = new List<string>();

                        for (int i = 0; i < uniqueEnemyList.Count; i++)
                        {
                            chaliceEnemiesString.Add(uniqueEnemyListc[i]);
                        }
                    }
                }
            }

            string dateNow = DateTime.Now.ToString("h:mm:ss tt");
            string butts = dateNow.Replace(":", "-");
            enemyLogFilePath = filePath + "\\Mod Files\\Logs. Don't Delete\\" + butts + "-EnemyLog.txt";
            if (logging)
                using (FileStream sw1 = File.Create(enemyLogFilePath))
                {

                }

            this.Dispatcher.Invoke(() =>
            {
                totalPercent = 25;
                currentTask = "Randomizing Enemies...";
                UpdateUI();
            });

            Randomize(filePath + "\\map\\mapstudio\\" + "m21_00_00_00.msb.dcx", unusedPlusBossList);
            Randomize(filePath + "\\map\\mapstudio\\" + "m21_01_00_00.msb.dcx", unusedPlusBossList);
            Randomize(filePath + "\\map\\mapstudio\\" + "m22_00_00_00.msb.dcx", unusedPlusBossList);
            Randomize(filePath + "\\map\\mapstudio\\" + "m23_00_00_00.msb.dcx", unusedPlusBossList);
            Randomize(filePath + "\\map\\mapstudio\\" + "m23_00_00_01.msb.dcx", unusedPlusBossList);
            Randomize(filePath + "\\map\\mapstudio\\" + "m24_00_00_00.msb.dcx", unusedPlusBossList);
            Randomize(filePath + "\\map\\mapstudio\\" + "m24_00_00_01.msb.dcx", unusedPlusBossList);
            Randomize(filePath + "\\map\\mapstudio\\" + "m24_01_00_00.msb.dcx", unusedPlusBossList);
            Randomize(filePath + "\\map\\mapstudio\\" + "m24_01_00_01.msb.dcx", unusedPlusBossList);
            Randomize(filePath + "\\map\\mapstudio\\" + "m24_01_00_11.msb.dcx", unusedPlusBossList);
            Randomize(filePath + "\\map\\mapstudio\\" + "m24_02_00_00.msb.dcx", unusedPlusBossList);
            Randomize(filePath + "\\map\\mapstudio\\" + "m24_02_00_01.msb.dcx", unusedPlusBossList);
            Randomize(filePath + "\\map\\mapstudio\\" + "m25_00_00_00.msb.dcx", unusedPlusBossList);
            Randomize(filePath + "\\map\\mapstudio\\" + "m26_00_00_00.msb.dcx", unusedPlusBossList);
            Randomize(filePath + "\\map\\mapstudio\\" + "m27_00_00_00.msb.dcx", unusedPlusBossList);
            Randomize(filePath + "\\map\\mapstudio\\" + "m27_00_00_01.msb.dcx", unusedPlusBossList);
            Randomize(filePath + "\\map\\mapstudio\\" + "m28_00_00_00.msb.dcx", unusedPlusBossList);
            Randomize(filePath + "\\map\\mapstudio\\" + "m28_00_00_01.msb.dcx", unusedPlusBossList);
            Randomize(filePath + "\\map\\mapstudio\\" + "m32_00_00_00.msb.dcx", unusedPlusBossList);
            Randomize(filePath + "\\map\\mapstudio\\" + "m32_00_00_01.msb.dcx", unusedPlusBossList);
            Randomize(filePath + "\\map\\mapstudio\\" + "m33_00_00_00.msb.dcx", unusedPlusBossList);
            Randomize(filePath + "\\map\\mapstudio\\" + "m34_00_00_00.msb.dcx", unusedPlusBossList);
            Randomize(filePath + "\\map\\mapstudio\\" + "m35_00_00_00.msb.dcx", unusedPlusBossList);
            Randomize(filePath + "\\map\\mapstudio\\" + "m36_00_00_00.msb.dcx", unusedPlusBossList);
            if (chaliceEnemies && randomizeEnemiesBool || oopsAll)
            {

                //Randomize(filePath + "\\map\\mapstudio\\" + "m29_10_90_00\\m29_10_90_00.msb.dcx", nonoList);
                Randomize(filePath + "\\map\\mapstudio\\" + "m29_10_90_00\\m29_10_90_01.msb.dcx", unusedPlusBossList);
                //Randomize(filePath + "\\map\\mapstudio\\" + "m29_20_90_00\\m29_20_90_00.msb.dcx", nonoList);
                Randomize(filePath + "\\map\\mapstudio\\" + "m29_20_90_00\\m29_20_90_01.msb.dcx", unusedPlusBossList);
                Randomize(filePath + "\\map\\mapstudio\\" + "m29_21_90_00\\m29_21_90_00.msb.dcx", unusedPlusBossList);
                //(filePath + "\\map\\mapstudio\\" + "m29_30_90_00\\m29_30_90_00.msb.dcx", nonoList);
                Randomize(filePath + "\\map\\mapstudio\\" + "m29_30_90_00\\m29_30_90_01.msb.dcx", unusedPlusBossList);
                Randomize(filePath + "\\map\\mapstudio\\" + "m29_31_90_00\\m29_31_90_00.msb.dcx", unusedPlusBossList);
                //Randomize(filePath + "\\map\\mapstudio\\" + "m29_40_90_00\\m29_40_90_00.msb.dcx", nonoList);
                Randomize(filePath + "\\map\\mapstudio\\" + "m29_40_90_00\\m29_40_90_01.msb.dcx", unusedPlusBossList);
                //Randomize(filePath + "\\map\\mapstudio\\" + "m29_42_90_00\\m29_42_90_00.msb.dcx", nonoList);
                Randomize(filePath + "\\map\\mapstudio\\" + "m29_42_90_00\\m29_42_90_01.msb.dcx", unusedPlusBossList);
                //Randomize(filePath + "\\map\\mapstudio\\" + "m29_50_90_00\\m29_50_90_00.msb.dcx", nonoList);
                Randomize(filePath + "\\map\\mapstudio\\" + "m29_50_90_00\\m29_50_90_01.msb.dcx", unusedPlusBossList);
                //Randomize(filePath + "\\map\\mapstudio\\" + "m29_52_90_00\\m29_52_90_00.msb.dcx", nonoList);
                Randomize(filePath + "\\map\\mapstudio\\" + "m29_52_90_00\\m29_52_90_01.msb.dcx", unusedPlusBossList);
                //Randomize(filePath + "\\map\\mapstudio\\" + "m29_53_90_00\\m29_53_90_00.msb.dcx", nonoList);
                Randomize(filePath + "\\map\\mapstudio\\" + "m29_53_90_00\\m29_53_90_01.msb.dcx", unusedPlusBossList);
                /*
                Randomize(filePath + "\\map\\mapstudio\\" + "m29_01_00_00\\m29_01_00_00.msb.dcx", unusedPlusBossList);
                Randomize(filePath + "\\map\\mapstudio\\" + "m29_01_00_00\\m29_01_00_33.msb.dcx", unusedPlusBossList);
                Randomize(filePath + "\\map\\mapstudio\\" + "m29_01_00_00\\m29_01_00_36.msb.dcx", unusedPlusBossList);
                Randomize(filePath + "\\map\\mapstudio\\" + "m29_01_00_00\\m29_01_00_45.msb.dcx", unusedPlusBossList);
                Randomize(filePath + "\\map\\mapstudio\\" + "m29_01_00_00\\m29_01_00_99.msb.dcx", unusedPlusBossList);
                Randomize(filePath + "\\map\\mapstudio\\" + "m29_01_02_00\\m29_01_02_03.msb.dcx", unusedPlusBossList);
                Randomize(filePath + "\\map\\mapstudio\\" + "m29_01_99_00\\m29_01_99_99.msb.dcx", unusedPlusBossList);
                Randomize(filePath + "\\map\\mapstudio\\" + "m29_09_00_00\\m29_09_00_00.msb.dcx", unusedPlusBossList);
                */

                if (GOBAdded)
                {
                    Randomize(filePath + "\\map\\mapstudio\\" + "m29_50_40_00\\m29_50_40_00.msb.dcx", unusedPlusBossList);
                }

            }

            enemyData = new List<string>();
            enemyDataRandomized = new List<string>();

            if (!oopsAll)
            {
                this.Dispatcher.Invoke(() =>
                {
                    totalPercent = 35;
                    currentTask = "Generating Boss List...";
                    UpdateUI();
                });

                if (excludeBossesBool)
                {
                    if (oopsBossString.Length > 5)
                    {
                        string tempOopsString = oopsBossString.Replace(" ", "");
                        int totalStrings = tempOopsString.Length / 5;
                        int startingNumber = 0;
                        for (int i = 0; i < totalStrings; i++)
                        {
                            oopsAllBossString.Add(tempOopsString.Substring(startingNumber, 5));
                            startingNumber = startingNumber + 5;
                        }
                    }
                    else if (oopsBossString.Length == 5)
                    {
                        oopsAllBossString.Add(oopsBossString);
                    }
                }

                GenerateBossList(filePath + "\\map\\mapstudio\\" + "m21_00_00_00.msb.dcx", unusedPlusBossList);
                GenerateBossList(filePath + "\\map\\mapstudio\\" + "m21_01_00_00.msb.dcx", unusedPlusBossList);
                GenerateBossList(filePath + "\\map\\mapstudio\\" + "m22_00_00_00.msb.dcx", unusedPlusBossList);
                GenerateBossList(filePath + "\\map\\mapstudio\\" + "m23_00_00_00.msb.dcx", unusedPlusBossList);
                //DoStuffBosses(filePath + "\\map\\mapstudio\\" + "m23_00_00_01.msb.dcx", nonoList);
                GenerateBossList(filePath + "\\map\\mapstudio\\" + "m24_00_00_00.msb.dcx", unusedPlusBossList);
                //DoStuffBosses(filePath + "\\map\\mapstudio\\" + "m24_00_00_01.msb.dcx", nonoList);
                GenerateBossList(filePath + "\\map\\mapstudio\\" + "m24_01_00_00.msb.dcx", unusedPlusBossList);
                //DoStuffBosses(filePath + "\\map\\mapstudio\\" + "m24_01_00_01.msb.dcx", nonoList);
                //DoStuffBosses(filePath + "\\map\\mapstudio\\" + "m24_01_00_11.msb.dcx", nonoList);
                GenerateBossList(filePath + "\\map\\mapstudio\\" + "m24_02_00_00.msb.dcx", unusedPlusBossList);
                //DoStuffBosses(filePath + "\\map\\mapstudio\\" + "m24_02_00_01.msb.dcx", nonoList);
                GenerateBossList(filePath + "\\map\\mapstudio\\" + "m25_00_00_00.msb.dcx", unusedPlusBossList);
                GenerateBossList(filePath + "\\map\\mapstudio\\" + "m26_00_00_00.msb.dcx", unusedPlusBossList);
                GenerateBossList(filePath + "\\map\\mapstudio\\" + "m27_00_00_00.msb.dcx", unusedPlusBossList);
                //DoStuffBosses(filePath + "\\map\\mapstudio\\" + "m27_00_00_01.msb.dcx", nonoList);
                GenerateBossList(filePath + "\\map\\mapstudio\\" + "m28_00_00_00.msb.dcx", unusedPlusBossList);
                //DoStuffBosses(filePath + "\\map\\mapstudio\\" + "m28_00_00_01.msb.dcx", nonoList);
                GenerateBossList(filePath + "\\map\\mapstudio\\" + "m32_00_00_00.msb.dcx", unusedPlusBossList);
                //DoStuffBosses(filePath + "\\map\\mapstudio\\" + "m32_00_00_01.msb.dcx", nonoList);
                GenerateBossList(filePath + "\\map\\mapstudio\\" + "m33_00_00_00.msb.dcx", unusedPlusBossList);
                GenerateBossList(filePath + "\\map\\mapstudio\\" + "m34_00_00_00.msb.dcx", unusedPlusBossList);
                GenerateBossList(filePath + "\\map\\mapstudio\\" + "m35_00_00_00.msb.dcx", unusedPlusBossList);
                GenerateBossList(filePath + "\\map\\mapstudio\\" + "m36_00_00_00.msb.dcx", unusedPlusBossList);
                if (chaliceBosses)
                {
                    GenerateBossList(filePath + "\\map\\mapstudio\\" + "m29_10_90_00\\m29_10_90_00.msb.dcx", unusedPlusBossList);
                    //DoStuffBosses(filePath + "\\map\\mapstudio\\" + "m29_10_90_00\\m29_10_90_01.msb.dcx", nonoList);
                    GenerateBossList(filePath + "\\map\\mapstudio\\" + "m29_20_90_00\\m29_20_90_00.msb.dcx", unusedPlusBossList);
                    //DoStuffBosses(filePath + "\\map\\mapstudio\\" + "m29_20_90_00\\m29_20_90_01.msb.dcx", nonoList);
                    GenerateBossList(filePath + "\\map\\mapstudio\\" + "m29_21_90_00\\m29_21_90_00.msb.dcx", unusedPlusBossList);
                    GenerateBossList(filePath + "\\map\\mapstudio\\" + "m29_30_90_00\\m29_30_90_00.msb.dcx", unusedPlusBossList);
                    //DoStuffBosses(filePath + "\\map\\mapstudio\\" + "m29_30_90_00\\m29_30_90_01.msb.dcx", nonoList);
                    GenerateBossList(filePath + "\\map\\mapstudio\\" + "m29_31_90_00\\m29_31_90_00.msb.dcx", unusedPlusBossList);
                    GenerateBossList(filePath + "\\map\\mapstudio\\" + "m29_40_90_00\\m29_40_90_00.msb.dcx", unusedPlusBossList);
                    //DoStuffBosses(filePath + "\\map\\mapstudio\\" + "m29_40_90_00\\m29_40_90_01.msb.dcx", nonoList);
                    GenerateBossList(filePath + "\\map\\mapstudio\\" + "m29_42_90_00\\m29_42_90_00.msb.dcx", unusedPlusBossList);
                    //DoStuffBosses(filePath + "\\map\\mapstudio\\" + "m29_42_90_00\\m29_42_90_01.msb.dcx", nonoList);
                    GenerateBossList(filePath + "\\map\\mapstudio\\" + "m29_50_90_00\\m29_50_90_00.msb.dcx", unusedPlusBossList);
                    //DoStuffBosses(filePath + "\\map\\mapstudio\\" + "m29_50_90_00\\m29_50_90_01.msb.dcx", nonoList);
                    GenerateBossList(filePath + "\\map\\mapstudio\\" + "m29_52_90_00\\m29_52_90_00.msb.dcx", unusedPlusBossList);
                    //DoStuffBosses(filePath + "\\map\\mapstudio\\" + "m29_52_90_00\\m29_52_90_01.msb.dcx", nonoList);
                    GenerateBossList(filePath + "\\map\\mapstudio\\" + "m29_53_90_00\\m29_53_90_00.msb.dcx", unusedPlusBossList);
                    //DoStuffBosses(filePath + "\\map\\mapstudio\\" + "m29_53_90_00\\m29_53_90_01.msb.dcx", nonoList);
                    if (GOBAdded)
                    {
                        GenerateBossList(filePath + "\\map\\mapstudio\\" + "m29_50_40_00\\m29_50_40_00.msb.dcx", unusedPlusBossList);
                    }
                }

                if (excludeBossesBool)
                {

                    if (insertBossesBool)
                    {
                        if (insertBossesString != null)
                        {
                            if (insertBossesString.Count > 0)
                            {
                                for (int i = insertBossesString.Count - 1; i > -1; i--)
                                {
                                    for (int j = 0; j < oopsAllBossString.Count; j++)
                                    {
                                        if (insertBossesString[i].Contains(oopsAllBossString[j]))
                                        {
                                            insertBossesString.RemoveAt(i);
                                        }
                                    }
                                }
                            }
                        }
                    }

                    if (enemyData != null)
                    {
                        if (enemyData.Count > 0)
                        {
                            for (int i = enemyData.Count - 1; i > -1; i--)
                            {
                                for (int j = 0; j < oopsAllBossString.Count; j++)
                                {
                                    if (enemyData[i].Contains(oopsAllBossString[j]))
                                    {
                                        enemyData.RemoveAt(i);
                                    }
                                }
                            }
                        }
                    }

                }

                chaliceBossParams.Add("210306015");

                List<string> uniqueEnemyList = enemyData.Distinct().ToList();
                enemyData = new List<string>();

                for (int i = 0; i < uniqueEnemyList.Count; i++)
                {
                    enemyData.Add(uniqueEnemyList[i]);
                }

                List<string> uniqueEnemyList2 = chaliceBossString.Distinct().ToList();
                chaliceBossString = new List<string>();

                for (int i = 0; i < uniqueEnemyList2.Count; i++)
                {
                    chaliceBossString.Add(uniqueEnemyList2[i]);
                }

                if (includeBosses)
                {
                    for (int i = 0; i < enemyData.Count; i++)
                    {
                        secondaryBosses.Add(enemyData[i]);
                        tertiaryBosses.Add(enemyData[i]);
                    }

                    if (chaliceBosses)
                    {
                        for (int i = 0; i < chaliceBossString.Count; i++)
                        {
                            combinedBossList.Add(chaliceBossString[i]);
                        }
                        for (int i = 0; i < enemyData.Count; i++)
                        {
                            combinedBossList.Add(enemyData[i]);
                        }
                    }

                    for (int i = 0; i < combinedBossList.Count; i++)
                    {
                        combinedBossList2.Add(combinedBossList[i]);
                    }

                    for (int i = 0; i < combinedBossList2.Count; i++)
                    {
                        for (int j = combinedBossList2.Count - 1; j >= 0; j--)
                        {
                            string modelString = combinedBossList2[i].Substring(combinedBossList2[i].Length - 5, 5);

                            if (combinedBossList2[j].Contains(modelString) && j != i)
                            {
                                combinedBossList2.RemoveAt(j);
                            }
                        }
                    }

                    while (secondaryBosses.Count > 0)
                    {
                        int index = universalRand.Next(0, secondaryBosses.Count);
                        enemyDataRandomized.Add(secondaryBosses[index]);
                        secondaryBosses.RemoveAt(index);
                    }



                }
            }
            dateNow = DateTime.Now.ToString("h:mm:ss tt");
            butts = dateNow.Replace(":", "-");
            bossLogFilePath = filePath + "\\Mod Files\\Logs. Don't Delete\\" + butts + "-BossLog.txt";
            if (logging)
                using (FileStream sw = File.Create(bossLogFilePath))
                {

                }

            dateNow = DateTime.Now.ToString("h:mm:ss tt");
            butts = dateNow.Replace(":", "-");
            dummyFilePath = filePath + "\\Mod Files\\Logs. Don't Delete\\" + butts + "-DummyLog.txt";
            if (logging)
                using (FileStream sw = File.Create(dummyFilePath))
                {

                }

            if (includeBosses)
            {
                this.Dispatcher.Invoke(() =>
                {
                    totalPercent = 45;
                    currentTask = "Randomizing Bosses...";
                    UpdateUI();
                });

                int thisOne = universalRand.Next(0, 3);

                if (addOrphan)
                {
                    if (!oopsAllBosses)
                    {
                        if (thisOne == 0)
                        {
                            AddOrphanPhaseOne(filePath + "\\map\\mapstudio\\" + "m24_02_00_01.msb.dcx", unusedPlusBossList);
                        }
                        else if (thisOne == 1)
                        {
                            AddOrphanPhaseOne(filePath + "\\map\\mapstudio\\" + "m24_01_00_01.msb.dcx", unusedPlusBossList);
                        }
                        else if (thisOne == 2)
                        {
                            AddOrphanPhaseOne(filePath + "\\map\\mapstudio\\" + "m34_00_00_00.msb.dcx", unusedPlusBossList);
                        }
                    }
                }

                if (logging)
                    using (StreamWriter writetext = File.AppendText(bossLogFilePath))
                    {
                        writetext.WriteLine("Number of bosses in combined boss list");
                        for (int i = 0; i < combinedBossList.Count; i++)
                        {
                            writetext.WriteLine(combinedBossList[i]);
                        }

                        writetext.WriteLine("Enemy Pool Count: " + chaliceBossString.Count + Environment.NewLine + Environment.NewLine);
                    }

                RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m24_00_00_01.msb.dcx", unusedPlusBossList);
                RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m24_02_00_01.msb.dcx", unusedPlusBossList);

                RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m21_00_00_00.msb.dcx", unusedPlusBossList);
                RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m21_01_00_00.msb.dcx", unusedPlusBossList);
                RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m22_00_00_00.msb.dcx", unusedPlusBossList);
                RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m23_00_00_00.msb.dcx", unusedPlusBossList);
                RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m23_00_00_01.msb.dcx", unusedPlusBossList);
                //RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m24_00_00_00.msb.dcx", nonoList);
                //RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m24_01_00_00.msb.dcx", nonoList);
                RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m24_01_00_01.msb.dcx", unusedPlusBossList);
                //RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m24_01_00_11.msb.dcx", nonoList);
                //RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m24_02_00_00.msb.dcx", nonoList);
                RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m25_00_00_00.msb.dcx", unusedPlusBossList);
                RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m26_00_00_00.msb.dcx", unusedPlusBossList);
                //RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m27_00_00_00.msb.dcx", nonoList);
                RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m27_00_00_01.msb.dcx", unusedPlusBossList);
                //RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m28_00_00_00.msb.dcx", nonoList);
                RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m28_00_00_01.msb.dcx", unusedPlusBossList);
                //RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m32_00_00_00.msb.dcx", nonoList);
                RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m32_00_00_01.msb.dcx", unusedPlusBossList);
                RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m33_00_00_00.msb.dcx", unusedPlusBossList);
                RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m34_00_00_00.msb.dcx", unusedPlusBossList);
                RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m35_00_00_00.msb.dcx", unusedPlusBossList);
                RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m36_00_00_00.msb.dcx", unusedPlusBossList);

                if (!oopsAllBosses)
                {
                    AddTheRest(filePath + "\\map\\mapstudio\\" + "m22_00_00_00.msb.dcx", unusedPlusBossList);
                    AddTheRest(filePath + "\\map\\mapstudio\\" + "m27_00_00_01.msb.dcx", unusedPlusBossList);
                    AddTheRest(filePath + "\\map\\mapstudio\\" + "m35_00_00_00.msb.dcx", unusedPlusBossList);
                }

                if (chaliceBosses || oopsAllBosses)
                {

                    //RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m29_10_90_00\\m29_10_90_00.msb.dcx", nonoList);
                    RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m29_10_90_00\\m29_10_90_01.msb.dcx", unusedPlusBossList);
                    //RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m29_20_90_00\\m29_20_90_00.msb.dcx", nonoList);
                    RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m29_20_90_00\\m29_20_90_01.msb.dcx", unusedPlusBossList);
                    RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m29_21_90_00\\m29_21_90_00.msb.dcx", unusedPlusBossList);
                    //RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m29_30_90_00\\m29_30_90_00.msb.dcx", nonoList);
                    RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m29_30_90_00\\m29_30_90_01.msb.dcx", unusedPlusBossList);
                    RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m29_31_90_00\\m29_31_90_00.msb.dcx", unusedPlusBossList);
                    //RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m29_40_90_00\\m29_40_90_00.msb.dcx", nonoList);
                    RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m29_40_90_00\\m29_40_90_01.msb.dcx", unusedPlusBossList);
                    //RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m29_42_90_00\\m29_42_90_00.msb.dcx", nonoList);
                    RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m29_42_90_00\\m29_42_90_01.msb.dcx", unusedPlusBossList);
                    //RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m29_50_90_00\\m29_50_90_00.msb.dcx", nonoList);
                    RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m29_50_90_00\\m29_50_90_01.msb.dcx", unusedPlusBossList);
                    //RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m29_52_90_00\\m29_52_90_00.msb.dcx", nonoList);
                    RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m29_52_90_00\\m29_52_90_01.msb.dcx", unusedPlusBossList);
                    //RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m29_53_90_00\\m29_53_90_00.msb.dcx", nonoList);
                    RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m29_53_90_00\\m29_53_90_01.msb.dcx", unusedPlusBossList);
                    if (GOBAdded)
                    {
                        RandomizeBosses(filePath + "\\map\\mapstudio\\" + "m29_50_40_00\\m29_50_40_00.msb.dcx", unusedPlusBossList);
                    }

                }
            }
            if (insertBossesBool)
            {
                List<string> uniqueLst = insertBossesString.Distinct().ToList();
                insertBossesString = new List<string>();

                for (int i = 0; i < uniqueLst.Count; i++)
                {
                    insertBossesString.Add(uniqueLst[i]);
                }



                if (logging)
                    using (StreamWriter writetext = File.AppendText(insertedBossPath))
                    {
                        writetext.WriteLine("INSERTING BOSSES STARTING BELOW THIS LINE");
                        writetext.WriteLine("" + Environment.NewLine + Environment.NewLine);
                    }

                this.Dispatcher.Invoke(() =>
                {
                    totalPercent = 55;
                    currentTask = "Replacing Enemies With Bosses...";
                    UpdateUI();
                });

                if (bossHemwick)
                {
                    InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m22_00_00_00.msb.dcx", unusedPlusBossList);
                }
                if (bossOldYharnam)
                {
                    InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m23_00_00_00.msb.dcx", unusedPlusBossList);
                    InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m23_00_00_01.msb.dcx", unusedPlusBossList);
                }
                if (bossCathedralWard)
                {
                    InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m24_00_00_00.msb.dcx", unusedPlusBossList);
                    InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m24_00_00_01.msb.dcx", unusedPlusBossList);
                }
                if (bossCentralYharnam)
                {
                    InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m24_01_00_00.msb.dcx", unusedPlusBossList);
                    InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m24_01_00_01.msb.dcx", unusedPlusBossList);
                    InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m24_01_00_11.msb.dcx", unusedPlusBossList);
                }
                if (bossUpperCathedralWard)
                {
                    InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m24_02_00_00.msb.dcx", unusedPlusBossList);
                    InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m24_02_00_01.msb.dcx", unusedPlusBossList);
                }
                if (bossCainhurst)
                {
                    InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m25_00_00_00.msb.dcx", unusedPlusBossList);
                }
                if (bossMensis)
                {
                    InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m26_00_00_00.msb.dcx", unusedPlusBossList);
                }
                if (bossForbiddenWoods)
                {
                    InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m27_00_00_00.msb.dcx", unusedPlusBossList);
                    InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m27_00_00_01.msb.dcx", unusedPlusBossList);
                }
                if (bossYahargul)
                {
                    InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m28_00_00_00.msb.dcx", unusedPlusBossList);
                    InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m28_00_00_01.msb.dcx", unusedPlusBossList);
                }
                if (bossByrgenwerthLecture)
                {
                    InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m32_00_00_00.msb.dcx", unusedPlusBossList);
                    InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m32_00_00_01.msb.dcx", unusedPlusBossList);
                }
                if (bossFrontier)
                {
                    InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m33_00_00_00.msb.dcx", unusedPlusBossList);
                }
                if (bossNightmare)
                {
                    InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m34_00_00_00.msb.dcx", unusedPlusBossList);
                }
                if (bossResearchHall)
                {
                    InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m35_00_00_00.msb.dcx", unusedPlusBossList);
                }
                if (bossHamlet)
                {
                    InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m36_00_00_00.msb.dcx", unusedPlusBossList);
                }

                if (chaliceBosses)
                {
                    if (bossChalices)
                    {
                        /*
                        //InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m29_10_90_00\\m29_10_90_00.msb.dcx", nonoList);
                        InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m29_10_90_00\\m29_10_90_01.msb.dcx", unusedPlusBossList);
                        //InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m29_20_90_00\\m29_20_90_00.msb.dcx", nonoList);
                        InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m29_20_90_00\\m29_20_90_01.msb.dcx", unusedPlusBossList);
                        InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m29_21_90_00\\m29_21_90_00.msb.dcx", unusedPlusBossList);
                        //InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m29_30_90_00\\m29_30_90_00.msb.dcx", nonoList);
                        InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m29_30_90_00\\m29_30_90_01.msb.dcx", unusedPlusBossList);
                        InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m29_31_90_00\\m29_31_90_00.msb.dcx", unusedPlusBossList);
                        //InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m29_40_90_00\\m29_40_90_00.msb.dcx", nonoList);
                        InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m29_40_90_00\\m29_40_90_01.msb.dcx", unusedPlusBossList);
                        //InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m29_42_90_00\\m29_42_90_00.msb.dcx", nonoList);
                        InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m29_42_90_00\\m29_42_90_01.msb.dcx", unusedPlusBossList);
                        //InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m29_50_90_00\\m29_50_90_00.msb.dcx", nonoList);
                        InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m29_50_90_00\\m29_50_90_01.msb.dcx", unusedPlusBossList);
                        //InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m29_52_90_00\\m29_52_90_00.msb.dcx", nonoList);
                        InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m29_52_90_00\\m29_52_90_01.msb.dcx", unusedPlusBossList);
                        //InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m29_53_90_00\\m29_53_90_00.msb.dcx", nonoList);
                        InsertBossesVoid(filePath + "\\map\\mapstudio\\" + "m29_53_90_00\\m29_53_90_01.msb.dcx", unusedPlusBossList);
                        */
                    }
                }
            }

            if (includeNPCs)
            {
                if (logging)
                    using (StreamWriter writetext = File.AppendText(randomizedNPCPath))
                    {
                        writetext.WriteLine("RANDOMIZED NPC'S STARTING BELOW THIS LINE");
                        writetext.WriteLine("" + Environment.NewLine + Environment.NewLine);
                    }

                this.Dispatcher.Invoke(() =>
                {
                    totalPercent = 65;
                    currentTask = "Generating NPC List...";
                    UpdateUI();
                });

                GenerateNPCList(filePath + "\\map\\mapstudio\\" + "m21_00_00_00.msb.dcx");
                GenerateNPCList(filePath + "\\map\\mapstudio\\" + "m21_01_00_00.msb.dcx");
                GenerateNPCList(filePath + "\\map\\mapstudio\\" + "m21_01_00_00.msb.dcx");
                GenerateNPCList(filePath + "\\map\\mapstudio\\" + "m22_00_00_00.msb.dcx");
                GenerateNPCList(filePath + "\\map\\mapstudio\\" + "m23_00_00_00.msb.dcx");
                GenerateNPCList(filePath + "\\map\\mapstudio\\" + "m23_00_00_01.msb.dcx");
                GenerateNPCList(filePath + "\\map\\mapstudio\\" + "m24_00_00_00.msb.dcx");
                GenerateNPCList(filePath + "\\map\\mapstudio\\" + "m24_00_00_01.msb.dcx");
                GenerateNPCList(filePath + "\\map\\mapstudio\\" + "m24_01_00_00.msb.dcx");
                GenerateNPCList(filePath + "\\map\\mapstudio\\" + "m24_01_00_01.msb.dcx");
                GenerateNPCList(filePath + "\\map\\mapstudio\\" + "m24_01_00_11.msb.dcx");
                GenerateNPCList(filePath + "\\map\\mapstudio\\" + "m24_02_00_00.msb.dcx");
                GenerateNPCList(filePath + "\\map\\mapstudio\\" + "m24_02_00_01.msb.dcx");
                GenerateNPCList(filePath + "\\map\\mapstudio\\" + "m25_00_00_00.msb.dcx");
                GenerateNPCList(filePath + "\\map\\mapstudio\\" + "m26_00_00_00.msb.dcx");
                GenerateNPCList(filePath + "\\map\\mapstudio\\" + "m27_00_00_00.msb.dcx");
                GenerateNPCList(filePath + "\\map\\mapstudio\\" + "m27_00_00_01.msb.dcx");
                GenerateNPCList(filePath + "\\map\\mapstudio\\" + "m28_00_00_00.msb.dcx");
                GenerateNPCList(filePath + "\\map\\mapstudio\\" + "m28_00_00_01.msb.dcx");
                GenerateNPCList(filePath + "\\map\\mapstudio\\" + "m32_00_00_00.msb.dcx");
                GenerateNPCList(filePath + "\\map\\mapstudio\\" + "m32_00_00_01.msb.dcx");
                GenerateNPCList(filePath + "\\map\\mapstudio\\" + "m33_00_00_00.msb.dcx");
                GenerateNPCList(filePath + "\\map\\mapstudio\\" + "m34_00_00_00.msb.dcx");
                GenerateNPCList(filePath + "\\map\\mapstudio\\" + "m35_00_00_00.msb.dcx");
                GenerateNPCList(filePath + "\\map\\mapstudio\\" + "m36_00_00_00.msb.dcx");
                GenerateNPCList(filePath + "\\map\\mapstudio\\" + "m29_10_90_00\\m29_10_90_01.msb.dcx");
                GenerateNPCList(filePath + "\\map\\mapstudio\\" + "m29_20_90_00\\m29_20_90_01.msb.dcx");
                GenerateNPCList(filePath + "\\map\\mapstudio\\" + "m29_21_90_00\\m29_21_90_00.msb.dcx");
                GenerateNPCList(filePath + "\\map\\mapstudio\\" + "m29_30_90_00\\m29_30_90_01.msb.dcx");
                GenerateNPCList(filePath + "\\map\\mapstudio\\" + "m29_31_90_00\\m29_31_90_00.msb.dcx");
                GenerateNPCList(filePath + "\\map\\mapstudio\\" + "m29_40_90_00\\m29_40_90_01.msb.dcx");
                GenerateNPCList(filePath + "\\map\\mapstudio\\" + "m29_42_90_00\\m29_42_90_01.msb.dcx");
                GenerateNPCList(filePath + "\\map\\mapstudio\\" + "m29_50_90_00\\m29_50_90_01.msb.dcx");
                GenerateNPCList(filePath + "\\map\\mapstudio\\" + "m29_52_90_00\\m29_52_90_01.msb.dcx");
                GenerateNPCList(filePath + "\\map\\mapstudio\\" + "m29_53_90_00\\m29_53_90_01.msb.dcx");

                for (int i = 0; i < npcEnemyList.Count; i++)
                {
                    string tempString = npcEnemyList[i].NPCParamID.ToString() + "*" + npcEnemyList[i].ThinkParamID.ToString() + "*" + npcEnemyList[i].UnkT07.ToString();
                    npcList.Add(npcEnemyList[i].NPCParamID.ToString() + "*" + npcEnemyList[i].ThinkParamID.ToString() + "*" + npcEnemyList[i].UnkT07.ToString());
                }

                for (int i = 0; i < npcList.Count; i++)
                {
                    if (npcList[i].Contains("*6300") || npcList[i].Contains("*6310")
                        || npcList[i].Contains("*6340") || npcList[i].Contains("*6350") || npcList[i].Contains("*6360")
                        || npcList[i].Contains("*6390") || npcList[i].Contains("*6395") || npcList[i].Contains("*6400")
                        || npcList[i].Contains("*6410") || npcList[i].Contains("*6420") || npcList[i].Contains("*6430")
                        || npcList[i].Contains("*6440") || npcList[i].Contains("*6450") || npcList[i].Contains("*6520")
                        || npcList[i].Contains("*6570") || npcList[i].Contains("*6580") || npcList[i].Contains("*6585")
                        || npcList[i].Contains("*6610") || npcList[i].Contains("*6630") || npcList[i].Contains("*7070")
                        || npcList[i].Contains("*7075"))
                    {
                        hostileNpcList.Add(npcList[i]);
                    }
                }

                this.Dispatcher.Invoke(() =>
                {
                    totalPercent = 75;
                    currentTask = "Randomizing NPC's...";
                    UpdateUI();
                });

                RandomizeNPCs(filePath + "\\map\\mapstudio\\" + "m21_00_00_00.msb.dcx");
                RandomizeNPCs(filePath + "\\map\\mapstudio\\" + "m21_01_00_00.msb.dcx");
                RandomizeNPCs(filePath + "\\map\\mapstudio\\" + "m21_01_00_00.msb.dcx");
                RandomizeNPCs(filePath + "\\map\\mapstudio\\" + "m22_00_00_00.msb.dcx");
                RandomizeNPCs(filePath + "\\map\\mapstudio\\" + "m23_00_00_00.msb.dcx");
                RandomizeNPCs(filePath + "\\map\\mapstudio\\" + "m23_00_00_01.msb.dcx");
                RandomizeNPCs(filePath + "\\map\\mapstudio\\" + "m24_00_00_00.msb.dcx");
                RandomizeNPCs(filePath + "\\map\\mapstudio\\" + "m24_00_00_01.msb.dcx");
                RandomizeNPCs(filePath + "\\map\\mapstudio\\" + "m24_01_00_00.msb.dcx");
                RandomizeNPCs(filePath + "\\map\\mapstudio\\" + "m24_01_00_01.msb.dcx");
                RandomizeNPCs(filePath + "\\map\\mapstudio\\" + "m24_01_00_11.msb.dcx");
                RandomizeNPCs(filePath + "\\map\\mapstudio\\" + "m24_02_00_00.msb.dcx");
                RandomizeNPCs(filePath + "\\map\\mapstudio\\" + "m24_02_00_01.msb.dcx");
                RandomizeNPCs(filePath + "\\map\\mapstudio\\" + "m25_00_00_00.msb.dcx");
                RandomizeNPCs(filePath + "\\map\\mapstudio\\" + "m26_00_00_00.msb.dcx");
                RandomizeNPCs(filePath + "\\map\\mapstudio\\" + "m27_00_00_00.msb.dcx");
                RandomizeNPCs(filePath + "\\map\\mapstudio\\" + "m27_00_00_01.msb.dcx");
                RandomizeNPCs(filePath + "\\map\\mapstudio\\" + "m28_00_00_00.msb.dcx");
                RandomizeNPCs(filePath + "\\map\\mapstudio\\" + "m28_00_00_01.msb.dcx");
                RandomizeNPCs(filePath + "\\map\\mapstudio\\" + "m32_00_00_00.msb.dcx");
                RandomizeNPCs(filePath + "\\map\\mapstudio\\" + "m32_00_00_01.msb.dcx");
                RandomizeNPCs(filePath + "\\map\\mapstudio\\" + "m33_00_00_00.msb.dcx");
                RandomizeNPCs(filePath + "\\map\\mapstudio\\" + "m34_00_00_00.msb.dcx");
                RandomizeNPCs(filePath + "\\map\\mapstudio\\" + "m35_00_00_00.msb.dcx");
                RandomizeNPCs(filePath + "\\map\\mapstudio\\" + "m36_00_00_00.msb.dcx");
            }

            if (easyMultiBossesBool)
            {
                EasyModes(filePath + "\\map\\mapstudio\\" + "m27_00_00_01.msb.dcx");
            }
            if (easyRomBool)
            {
                EasyModes(filePath + "\\map\\mapstudio\\" + "m32_00_00_00.msb.dcx");
                EasyModes(filePath + "\\map\\mapstudio\\" + "m32_00_00_01.msb.dcx");
            }
            if (easyFailuresBool)
            {
                EasyModes(filePath + "\\map\\mapstudio\\" + "m35_00_00_00.msb.dcx");
            }
            if (easyWitchesBool)
            {
                EasyModes(filePath + "\\map\\mapstudio\\" + "m24_02_00_00.msb.dcx");
                EasyModes(filePath + "\\map\\mapstudio\\" + "m24_02_00_01.msb.dcx");
            }

            if (randomizeItemLots)
            {
                nonoItemLots.Add(2600550); //Evil Eye Bridge key (key in nightmare of mensis?)
                nonoItemLots.Add(2400450); //The key to the Old Town (gascoigne key?)
                nonoItemLots.Add(3500800); //The key to the dungeon usually door (braidor key?)
                keyItemLots.Add(2800290); //The key to Cathedral Street C (UCW key)
                keyItemLots.Add(3200720); //The key to nightmare classroom (byrgenworth lower floor key)
                keyItemLots.Add(3200810); //Veranda of key (key to rom fight)
                keyItemLots.Add(2410990); //Invitation to the castle (cainhurst summons)
                keyItemLots.Add(3502000); //Parish length Ω Startup Item (laurence skull)
                keyItemLots.Add(3401810); //Altar Elevator Startup Item (eye pendant)
                workshopItemsList.Add(2411000); //Blood gem workshop tool (chest after gascoigne)
                workshopItemsList.Add(2200360); //Rune tool (after witches)

                for (int i = 0; i < keyItemLots.Count; i++)
                {
                    nonoItemLots.Add(keyItemLots[i]);
                }

                if (!workshopBool)
                {
                    for (int i = 0; i < workshopItemsList.Count; i++)
                    {
                        nonoItemLots.Add(workshopItemsList[i]);
                    }
                }

                this.Dispatcher.Invoke(() =>
                {
                    totalPercent = 85;
                    currentTask = "Generating Item List...";
                    UpdateUI();
                });

                //GenerateItemLotList(filePath + "\\map\\mapstudio\\" + "m21_00_00_00.msb.dcx");
                GenerateItemLotList(filePath + "\\map\\mapstudio\\" + "m21_01_00_00.msb.dcx");
                GenerateItemLotList(filePath + "\\map\\mapstudio\\" + "m21_01_00_00.msb.dcx");
                GenerateItemLotList(filePath + "\\map\\mapstudio\\" + "m22_00_00_00.msb.dcx");
                GenerateItemLotList(filePath + "\\map\\mapstudio\\" + "m23_00_00_00.msb.dcx");
                GenerateItemLotList(filePath + "\\map\\mapstudio\\" + "m23_00_00_01.msb.dcx");
                GenerateItemLotList(filePath + "\\map\\mapstudio\\" + "m24_00_00_00.msb.dcx");
                GenerateItemLotList(filePath + "\\map\\mapstudio\\" + "m24_00_00_01.msb.dcx");
                GenerateItemLotList(filePath + "\\map\\mapstudio\\" + "m24_01_00_00.msb.dcx");
                GenerateItemLotList(filePath + "\\map\\mapstudio\\" + "m24_01_00_01.msb.dcx");
                GenerateItemLotList(filePath + "\\map\\mapstudio\\" + "m24_01_00_11.msb.dcx");
                GenerateItemLotList(filePath + "\\map\\mapstudio\\" + "m24_02_00_00.msb.dcx");
                GenerateItemLotList(filePath + "\\map\\mapstudio\\" + "m24_02_00_01.msb.dcx");
                GenerateItemLotList(filePath + "\\map\\mapstudio\\" + "m25_00_00_00.msb.dcx");
                GenerateItemLotList(filePath + "\\map\\mapstudio\\" + "m26_00_00_00.msb.dcx");
                GenerateItemLotList(filePath + "\\map\\mapstudio\\" + "m27_00_00_00.msb.dcx");
                GenerateItemLotList(filePath + "\\map\\mapstudio\\" + "m27_00_00_01.msb.dcx");
                GenerateItemLotList(filePath + "\\map\\mapstudio\\" + "m28_00_00_00.msb.dcx");
                GenerateItemLotList(filePath + "\\map\\mapstudio\\" + "m28_00_00_01.msb.dcx");
                GenerateItemLotList(filePath + "\\map\\mapstudio\\" + "m32_00_00_00.msb.dcx");
                GenerateItemLotList(filePath + "\\map\\mapstudio\\" + "m32_00_00_01.msb.dcx");
                GenerateItemLotList(filePath + "\\map\\mapstudio\\" + "m33_00_00_00.msb.dcx");
                GenerateItemLotList(filePath + "\\map\\mapstudio\\" + "m34_00_00_00.msb.dcx");
                GenerateItemLotList(filePath + "\\map\\mapstudio\\" + "m35_00_00_00.msb.dcx");
                GenerateItemLotList(filePath + "\\map\\mapstudio\\" + "m36_00_00_00.msb.dcx");

                this.Dispatcher.Invoke(() =>
                {
                    totalPercent = 90;
                    currentTask = "Randomizing Items...";
                    UpdateUI();
                });

                //RandomizeItemLots(filePath + "\\map\\mapstudio\\" + "m21_00_00_00.msb.dcx");
                RandomizeItemLots(filePath + "\\map\\mapstudio\\" + "m21_01_00_00.msb.dcx");
                RandomizeItemLots(filePath + "\\map\\mapstudio\\" + "m21_01_00_00.msb.dcx");
                RandomizeItemLots(filePath + "\\map\\mapstudio\\" + "m22_00_00_00.msb.dcx");
                RandomizeItemLots(filePath + "\\map\\mapstudio\\" + "m23_00_00_00.msb.dcx");
                RandomizeItemLots(filePath + "\\map\\mapstudio\\" + "m23_00_00_01.msb.dcx");
                RandomizeItemLots(filePath + "\\map\\mapstudio\\" + "m24_00_00_00.msb.dcx");
                RandomizeItemLots(filePath + "\\map\\mapstudio\\" + "m24_00_00_01.msb.dcx");
                RandomizeItemLots(filePath + "\\map\\mapstudio\\" + "m24_01_00_00.msb.dcx");
                RandomizeItemLots(filePath + "\\map\\mapstudio\\" + "m24_01_00_01.msb.dcx");
                RandomizeItemLots(filePath + "\\map\\mapstudio\\" + "m24_01_00_11.msb.dcx");
                RandomizeItemLots(filePath + "\\map\\mapstudio\\" + "m24_02_00_00.msb.dcx");
                RandomizeItemLots(filePath + "\\map\\mapstudio\\" + "m24_02_00_01.msb.dcx");
                RandomizeItemLots(filePath + "\\map\\mapstudio\\" + "m25_00_00_00.msb.dcx");
                RandomizeItemLots(filePath + "\\map\\mapstudio\\" + "m26_00_00_00.msb.dcx");
                RandomizeItemLots(filePath + "\\map\\mapstudio\\" + "m27_00_00_00.msb.dcx");
                RandomizeItemLots(filePath + "\\map\\mapstudio\\" + "m27_00_00_01.msb.dcx");
                RandomizeItemLots(filePath + "\\map\\mapstudio\\" + "m28_00_00_00.msb.dcx");
                RandomizeItemLots(filePath + "\\map\\mapstudio\\" + "m28_00_00_01.msb.dcx");
                RandomizeItemLots(filePath + "\\map\\mapstudio\\" + "m32_00_00_00.msb.dcx");
                RandomizeItemLots(filePath + "\\map\\mapstudio\\" + "m32_00_00_01.msb.dcx");
                RandomizeItemLots(filePath + "\\map\\mapstudio\\" + "m33_00_00_00.msb.dcx");
                RandomizeItemLots(filePath + "\\map\\mapstudio\\" + "m34_00_00_00.msb.dcx");
                RandomizeItemLots(filePath + "\\map\\mapstudio\\" + "m35_00_00_00.msb.dcx");
                RandomizeItemLots(filePath + "\\map\\mapstudio\\" + "m36_00_00_00.msb.dcx");
            }

            PermaDarknessFunction(filePath + "\\event\\common.emevd.dcx");

            numbersInParamToRandomize = new List<int>();
            gunsToReplace = new List<int>();
            gunsToReplace.Add(6000000);
            gunsToReplace.Add(6000100);
            gunsToReplace.Add(6000200);
            gunsToReplace.Add(6000300);
            gunsToReplace.Add(6000400);
            gunsToReplace.Add(6000500);
            gunsToReplace.Add(6000600);
            gunsToReplace.Add(6000700);
            gunsToReplace.Add(6000800);
            gunsToReplace.Add(6000900);
            gunsToReplace.Add(6001000);
            gunsToReplace.Add(6100000);
            gunsToReplace.Add(6100100);
            gunsToReplace.Add(6100200);
            gunsToReplace.Add(6100300);
            gunsToReplace.Add(6100400);
            gunsToReplace.Add(6100500);
            gunsToReplace.Add(6100600);
            gunsToReplace.Add(6100700);
            gunsToReplace.Add(6100800);
            gunsToReplace.Add(6100900);
            gunsToReplace.Add(6101000);
            gunsToReplace.Add(14000000);
            gunsToReplace.Add(14000100);
            gunsToReplace.Add(14000200);
            gunsToReplace.Add(14000300);
            gunsToReplace.Add(14000400);
            gunsToReplace.Add(14000500);
            gunsToReplace.Add(14000600);
            gunsToReplace.Add(14000700);
            gunsToReplace.Add(14000800);
            gunsToReplace.Add(14000900);
            gunsToReplace.Add(14001000);
            gunsToReplace.Add(14100000);
            gunsToReplace.Add(14100100);
            gunsToReplace.Add(14100200);
            gunsToReplace.Add(14100300);
            gunsToReplace.Add(14100400);
            gunsToReplace.Add(14100500);
            gunsToReplace.Add(14100600);
            gunsToReplace.Add(14100700);
            gunsToReplace.Add(14100800);
            gunsToReplace.Add(14100900);
            gunsToReplace.Add(14101000);
            gunsToReplace.Add(14200000);
            gunsToReplace.Add(14200100);
            gunsToReplace.Add(14200200);
            gunsToReplace.Add(14200300);
            gunsToReplace.Add(14200400);
            gunsToReplace.Add(14200500);
            gunsToReplace.Add(14200600);
            gunsToReplace.Add(14200700);
            gunsToReplace.Add(14200800);
            gunsToReplace.Add(14200900);
            gunsToReplace.Add(14201000);
            gunsToReplace.Add(15000000);
            gunsToReplace.Add(15000100);
            gunsToReplace.Add(15000200);
            gunsToReplace.Add(15000300);
            gunsToReplace.Add(15000400);
            gunsToReplace.Add(15000500);
            gunsToReplace.Add(15000600);
            gunsToReplace.Add(15000700);
            gunsToReplace.Add(15000800);
            gunsToReplace.Add(15000900);
            gunsToReplace.Add(15001000);
            gunsToReplace.Add(18000000);
            gunsToReplace.Add(18000100);
            gunsToReplace.Add(18000200);
            gunsToReplace.Add(18000300);
            gunsToReplace.Add(18000400);
            gunsToReplace.Add(18000500);
            gunsToReplace.Add(18000600);
            gunsToReplace.Add(18000700);
            gunsToReplace.Add(18000800);
            gunsToReplace.Add(18000900);
            gunsToReplace.Add(18001000);
            gunsToReplace.Add(18100000);
            gunsToReplace.Add(18100100);
            gunsToReplace.Add(18100200);
            gunsToReplace.Add(18100300);
            gunsToReplace.Add(18100400);
            gunsToReplace.Add(18100500);
            gunsToReplace.Add(18100600);
            gunsToReplace.Add(18100700);
            gunsToReplace.Add(18100800);
            gunsToReplace.Add(18100900);
            gunsToReplace.Add(18101000);
            gunsToReplace.Add(33000000);
            gunsToReplace.Add(33000100);
            gunsToReplace.Add(33000200);
            gunsToReplace.Add(33000300);
            gunsToReplace.Add(33000400);
            gunsToReplace.Add(33000500);
            gunsToReplace.Add(33000600);
            gunsToReplace.Add(33000700);
            gunsToReplace.Add(33000800);
            gunsToReplace.Add(33000900);
            gunsToReplace.Add(33001000);
            gunsToReplace.Add(35000000);
            gunsToReplace.Add(35000100);
            gunsToReplace.Add(35000200);
            gunsToReplace.Add(35000300);
            gunsToReplace.Add(35000400);
            gunsToReplace.Add(35000500);
            gunsToReplace.Add(35000600);
            gunsToReplace.Add(35000700);
            gunsToReplace.Add(35000800);
            gunsToReplace.Add(35000900);
            gunsToReplace.Add(35001000);
            gunsToReplace.Add(36000000);
            gunsToReplace.Add(36000100);
            gunsToReplace.Add(36000200);
            gunsToReplace.Add(36000300);
            gunsToReplace.Add(36000400);
            gunsToReplace.Add(36000500);
            gunsToReplace.Add(36000600);
            gunsToReplace.Add(36000700);
            gunsToReplace.Add(36000800);
            gunsToReplace.Add(36000900);
            gunsToReplace.Add(36001000);
            numbersInParamToRandomize.Add(3);
            numbersInParamToRandomize.Add(8);
            numbersInParamToRandomize.Add(9);
            numbersInParamToRandomize.Add(10);
            numbersInParamToRandomize.Add(11);
            numbersInParamToRandomize.Add(12);
            numbersInParamToRandomize.Add(13);
            numbersInParamToRandomize.Add(14);
            numbersInParamToRandomize.Add(15);
            numbersInParamToRandomize.Add(19);
            numbersInParamToRandomize.Add(20);
            numbersInParamToRandomize.Add(21);
            numbersInParamToRandomize.Add(51);
            numbersInParamToRandomize.Add(52);
            numbersInParamToRandomize.Add(53);
            numbersInParamToRandomize.Add(54);
            numbersInParamToRandomize.Add(55);
            numbersInParamToRandomize.Add(56);
            numbersInParamToRandomize.Add(57);
            numbersInParamToRandomize.Add(58);
            numbersInParamToRandomize.Add(59);
            numbersInParamToRandomize.Add(60);
            numbersInParamToRandomize.Add(61);
            numbersInParamToRandomize.Add(62);
            numbersInParamToRandomize.Add(63);
            numbersInParamToRandomize.Add(64);
            numbersInParamToRandomize.Add(65);
            numbersInParamToRandomize.Add(66);
            numbersInParamToRandomize.Add(67);
            numbersInParamToRandomize.Add(68);
            numbersInParamToRandomize.Add(69);
            numbersInParamToRandomize.Add(70);
            numbersInParamToRandomize.Add(76);
            numbersInParamToRandomize.Add(77);
            numbersInParamToRandomize.Add(78);
            numbersInParamToRandomize.Add(79);
            numbersInParamToRandomize.Add(80);
            numbersInParamToRandomize.Add(81);
            numbersInParamToRandomize.Add(82);
            numbersInParamToRandomize.Add(90);
            numbersInParamToRandomize.Add(91);
            numbersInParamToRandomize.Add(92);
            numbersInParamToRandomize.Add(93);
            numbersInParamToRandomize.Add(94);
            numbersInParamToRandomize.Add(95);
            numbersInParamToRandomize.Add(96);
            numbersInParamToRandomize.Add(157);
            numbersInParamToRandomize.Add(158);

            if (meleeMoveset)
            {
                RandomizeMeleeMoveset("EquipParamWeapon", numbersInParamToRandomize, gunsToReplace);
            }

            if (gunMoveset)
            {
                RandomizeGunMoveset("EquipParamWeapon", numbersInParamToRandomize, gunsToReplace);
            }

            if (enemyDropBool)
            {
                RandomizeItemDrops();
            }

            if (shopBool || startingWeaponsOnlyBool)
            {
                RandomizeShopItems();
            }

            if (vfxBool)
            {
                AiSoundParamRandomizer();
                //VfxRandomizer("GemGenParam");
                //VfxRandomizer("Wind");
                //VfxRandomizer("KnockBackParam");
                //VfxRandomizer("Bullet");
                //BulletRandomizer();
                //VfxRandomizer("AiSoundParam");
            }

            if (teamTypeBool)
            {
                TeamTypeRando();
            }

            if (faceBool)
            {
                VfxRandomizer("FaceGenParam");
                VfxRandomizer("FaceParam");
            }

            if (bloodBool)
            {
                DecalRandomizer();
            }

            if (gemBool)
            {
                GemGenParamRandomizer();
            }

            if (talkBool)
            {
                TalkRandomizer();
            }

            //CutEnemyScan();

            string dateNoww = DateTime.Now.ToString("h:mm:ss tt");
            string buttss = dateNoww.Replace(":", "-");
            scaleLogFile = filePath + "\\Mod Files\\Logs. Don't Delete\\" + buttss + "-ScaleLog.txt";
            if (logging)
                using (FileStream sw1 = File.Create(scaleLogFile))
                {

                }

            this.Dispatcher.Invoke(() =>
            {
                totalPercent = 98;
                currentTask = "Scaling All Enemies And Bosses...";
                UpdateUI();
            });

            if (!noScalingBool)
            {
                if (!customScaling)
                {
                    dreamScale = 20;
                    hemwickScale = 7;
                    cathedralScale = 21;
                    upperScale = 14;
                    mensisScale = 17;
                    yahargulScale = 13;
                    frontierScale = 10;
                    researchScale = 27;
                    oldScale = 4;
                    centralScale = 1;
                    cainhurstScale = 12;
                    woodsScale = 9;
                    byrgenwerthScale = 11;
                    nightmareScale = 24;
                    hamletScale = 29;
                }
                else
                {

                    string huntersDreamIndex = "";
                    string hemwickIndex = "";
                    string cathedralWardIndex = "";
                    string upperIndex = "";
                    string mensisIndex = "";
                    string yahargulIndex = "";
                    string frontierIndex = "";
                    string researchIndex = "";
                    string oldIndex = "";
                    string centralIndex = "";
                    string cainhurstIndex = "";
                    string woodsIndex = "";
                    string byrgenwerthIndex = "";
                    string huntersNightmareIndex = "";
                    string hamletIndex = "";
                    //var window = (Window1)Application.maind;
                    this.Dispatcher.Invoke(() =>
                    {
                        huntersDreamIndex = win1.HuntersDreamBox.Text;
                        hemwickIndex = win1.HemwickBox.Text;
                        cathedralWardIndex = win1.CathedralBox.Text;
                        upperIndex = win1.UpperBox.Text;
                        mensisIndex = win1.MensisBox.Text;
                        yahargulIndex = win1.YahargulBox.Text;
                        frontierIndex = win1.FrontierBox.Text;
                        researchIndex = win1.ResearchBox.Text;
                        oldIndex = win1.OldBox.Text;
                        centralIndex = win1.CentralBox.Text;
                        cainhurstIndex = win1.CainhurstBox.Text;
                        woodsIndex = win1.WoodsBox.Text;
                        byrgenwerthIndex = win1.ByrgenwerthBox.Text;
                        huntersNightmareIndex = win1.HuntersNightmareBox.Text;
                        hamletIndex = win1.HamletBox.Text;
                    });

                    ////public int dreamScale;
                    //dreamScale = win1.dreamScale;
                    ////public int hemwickScale;
                    //hemwickScale = win1.hemwickScale;
                    ////public int cathedralScale;
                    //cathedralScale = win1.cathedralScale;
                    ////public int upperScale;
                    //upperScale = win1.upperScale;
                    ////public int mensisScale;
                    //mensisScale = win1.mensisScale;
                    ////public int yahargulScale;
                    //yahargulScale = win1.yahargulScale;
                    ////public int frontierScale;
                    //frontierScale = win1.frontierScale;
                    ////public int researchScale;
                    //researchScale = win1.researchScale;
                    ////public int oldScale;
                    //oldScale = win1.oldScale;
                    ////public int centralScale;
                    //centralScale = win1.centralScale;
                    ////public int cainhurstScale;
                    //cainhurstScale = win1.cainhurstScale;
                    ////public int woodsScale;
                    //woodsScale = win1.woodsScale;
                    ////public int byrgenwerthScale;
                    //byrgenwerthScale = win1.byrgenwerthScale;
                    ////public int nightmareScale;
                    //nightmareScale = win1.nightmareScale;
                    ////public int hamletScale;
                    //hamletScale = win1.hamletScale;

                    switch (huntersDreamIndex)
                    {
                        case "Hunter's Dream":
                            dreamScale = 20;
                            break;
                        case "Hemwick Charnel Lane":
                            dreamScale = 7;
                            break;
                        case "Cathedral Ward":
                            dreamScale = 21;
                            break;
                        case "Upper Cathedral Ward":
                            dreamScale = 14;
                            break;
                        case "Nightmare of Mensis":
                            dreamScale = 17;
                            break;
                        case "Yahargul":
                            dreamScale = 13;
                            break;
                        case "Nightmare Frontier":
                            dreamScale = 10;
                            break;
                        case "Research Hall":
                            dreamScale = 27;
                            break;
                        case "Old Yharnam":
                            dreamScale = 4;
                            break;
                        case "Central Yharnam":
                            dreamScale = 1;
                            break;
                        case "Cainhurst":
                            dreamScale = 12;
                            break;
                        case "Forbidden Woods":
                            dreamScale = 9;
                            break;
                        case "Byrgenwerth + Lecture hall":
                            dreamScale = 11;
                            break;
                        case "Hunter's Nightmare":
                            dreamScale = 24;
                            break;
                        case "Fishing Hamlet":
                            dreamScale = 29;
                            break;
                        default:
                            break;
                    }

                    //public string hemwickIndex;
                    switch (hemwickIndex)
                    {
                        case "Hunter's Dream":
                            hemwickScale = 20;
                            break;
                        case "Hemwick Charnel Lane":
                            hemwickScale = 7;
                            break;
                        case "Cathedral Ward":
                            hemwickScale = 21;
                            break;
                        case "Upper Cathedral Ward":
                            hemwickScale = 14;
                            break;
                        case "Nightmare of Mensis":
                            hemwickScale = 17;
                            break;
                        case "Yahargul":
                            hemwickScale = 13;
                            break;
                        case "Nightmare Frontier":
                            hemwickScale = 10;
                            break;
                        case "Research Hall":
                            hemwickScale = 27;
                            break;
                        case "Old Yharnam":
                            hemwickScale = 4;
                            break;
                        case "Central Yharnam":
                            hemwickScale = 1;
                            break;
                        case "Cainhurst":
                            hemwickScale = 12;
                            break;
                        case "Forbidden Woods":
                            hemwickScale = 9;
                            break;
                        case "Byrgenwerth + Lecture hall":
                            hemwickScale = 11;
                            break;
                        case "Hunter's Nightmare":
                            hemwickScale = 24;
                            break;
                        case "Fishing Hamlet":
                            hemwickScale = 29;
                            break;
                        default:
                            break;
                    }
                    //public string cathedralWardIndex;
                    switch (cathedralWardIndex)
                    {
                        case "Hunter's Dream":
                            cathedralScale = 20;
                            break;
                        case "Hemwick Charnel Lane":
                            cathedralScale = 7;
                            break;
                        case "Cathedral Ward":
                            cathedralScale = 21;
                            break;
                        case "Upper Cathedral Ward":
                            cathedralScale = 14;
                            break;
                        case "Nightmare of Mensis":
                            cathedralScale = 17;
                            break;
                        case "Yahargul":
                            cathedralScale = 13;
                            break;
                        case "Nightmare Frontier":
                            cathedralScale = 10;
                            break;
                        case "Research Hall":
                            cathedralScale = 27;
                            break;
                        case "Old Yharnam":
                            cathedralScale = 4;
                            break;
                        case "Central Yharnam":
                            cathedralScale = 1;
                            break;
                        case "Cainhurst":
                            cathedralScale = 12;
                            break;
                        case "Forbidden Woods":
                            cathedralScale = 9;
                            break;
                        case "Byrgenwerth + Lecture hall":
                            cathedralScale = 11;
                            break;
                        case "Hunter's Nightmare":
                            cathedralScale = 24;
                            break;
                        case "Fishing Hamlet":
                            cathedralScale = 29;
                            break;
                        default:
                            break;
                    }
                    //public string upperIndex;
                    switch (upperIndex)
                    {
                        case "Hunter's Dream":
                            upperScale = 20;
                            break;
                        case "Hemwick Charnel Lane":
                            upperScale = 7;
                            break;
                        case "Cathedral Ward":
                            upperScale = 21;
                            break;
                        case "Upper Cathedral Ward":
                            upperScale = 14;
                            break;
                        case "Nightmare of Mensis":
                            upperScale = 17;
                            break;
                        case "Yahargul":
                            upperScale = 13;
                            break;
                        case "Nightmare Frontier":
                            upperScale = 10;
                            break;
                        case "Research Hall":
                            upperScale = 27;
                            break;
                        case "Old Yharnam":
                            upperScale = 4;
                            break;
                        case "Central Yharnam":
                            upperScale = 1;
                            break;
                        case "Cainhurst":
                            upperScale = 12;
                            break;
                        case "Forbidden Woods":
                            upperScale = 9;
                            break;
                        case "Byrgenwerth + Lecture hall":
                            upperScale = 11;
                            break;
                        case "Hunter's Nightmare":
                            upperScale = 24;
                            break;
                        case "Fishing Hamlet":
                            upperScale = 29;
                            break;
                        default:
                            break;
                    }
                    //public string mensisIndex;
                    switch (mensisIndex)
                    {
                        case "Hunter's Dream":
                            mensisScale = 20;
                            break;
                        case "Hemwick Charnel Lane":
                            mensisScale = 7;
                            break;
                        case "Cathedral Ward":
                            mensisScale = 21;
                            break;
                        case "Upper Cathedral Ward":
                            mensisScale = 14;
                            break;
                        case "Nightmare of Mensis":
                            mensisScale = 17;
                            break;
                        case "Yahargul":
                            mensisScale = 13;
                            break;
                        case "Nightmare Frontier":
                            mensisScale = 10;
                            break;
                        case "Research Hall":
                            mensisScale = 27;
                            break;
                        case "Old Yharnam":
                            mensisScale = 4;
                            break;
                        case "Central Yharnam":
                            mensisScale = 1;
                            break;
                        case "Cainhurst":
                            mensisScale = 12;
                            break;
                        case "Forbidden Woods":
                            mensisScale = 9;
                            break;
                        case "Byrgenwerth + Lecture hall":
                            mensisScale = 11;
                            break;
                        case "Hunter's Nightmare":
                            mensisScale = 24;
                            break;
                        case "Fishing Hamlet":
                            mensisScale = 29;
                            break;
                        default:
                            break;
                    }
                    //public string yahargulIndex;
                    switch (yahargulIndex)
                    {
                        case "Hunter's Dream":
                            yahargulScale = 20;
                            break;
                        case "Hemwick Charnel Lane":
                            yahargulScale = 7;
                            break;
                        case "Cathedral Ward":
                            yahargulScale = 21;
                            break;
                        case "Upper Cathedral Ward":
                            yahargulScale = 14;
                            break;
                        case "Nightmare of Mensis":
                            yahargulScale = 17;
                            break;
                        case "Yahargul":
                            yahargulScale = 13;
                            break;
                        case "Nightmare Frontier":
                            yahargulScale = 10;
                            break;
                        case "Research Hall":
                            yahargulScale = 27;
                            break;
                        case "Old Yharnam":
                            yahargulScale = 4;
                            break;
                        case "Central Yharnam":
                            yahargulScale = 1;
                            break;
                        case "Cainhurst":
                            yahargulScale = 12;
                            break;
                        case "Forbidden Woods":
                            yahargulScale = 9;
                            break;
                        case "Byrgenwerth + Lecture hall":
                            yahargulScale = 11;
                            break;
                        case "Hunter's Nightmare":
                            yahargulScale = 24;
                            break;
                        case "Fishing Hamlet":
                            yahargulScale = 29;
                            break;
                        default:
                            break;
                    }
                    //public string frontierIndex;
                    switch (frontierIndex)
                    {
                        case "Hunter's Dream":
                            frontierScale = 20;
                            break;
                        case "Hemwick Charnel Lane":
                            frontierScale = 7;
                            break;
                        case "Cathedral Ward":
                            frontierScale = 21;
                            break;
                        case "Upper Cathedral Ward":
                            frontierScale = 14;
                            break;
                        case "Nightmare of Mensis":
                            frontierScale = 17;
                            break;
                        case "Yahargul":
                            frontierScale = 13;
                            break;
                        case "Nightmare Frontier":
                            frontierScale = 10;
                            break;
                        case "Research Hall":
                            frontierScale = 27;
                            break;
                        case "Old Yharnam":
                            frontierScale = 4;
                            break;
                        case "Central Yharnam":
                            frontierScale = 1;
                            break;
                        case "Cainhurst":
                            frontierScale = 12;
                            break;
                        case "Forbidden Woods":
                            frontierScale = 9;
                            break;
                        case "Byrgenwerth + Lecture hall":
                            frontierScale = 11;
                            break;
                        case "Hunter's Nightmare":
                            frontierScale = 24;
                            break;
                        case "Fishing Hamlet":
                            frontierScale = 29;
                            break;
                        default:
                            break;
                    }
                    //public string researchIndex;
                    switch (researchIndex)
                    {
                        case "Hunter's Dream":
                            researchScale = 20;
                            break;
                        case "Hemwick Charnel Lane":
                            researchScale = 7;
                            break;
                        case "Cathedral Ward":
                            researchScale = 21;
                            break;
                        case "Upper Cathedral Ward":
                            researchScale = 14;
                            break;
                        case "Nightmare of Mensis":
                            researchScale = 17;
                            break;
                        case "Yahargul":
                            researchScale = 13;
                            break;
                        case "Nightmare Frontier":
                            researchScale = 10;
                            break;
                        case "Research Hall":
                            researchScale = 27;
                            break;
                        case "Old Yharnam":
                            researchScale = 4;
                            break;
                        case "Central Yharnam":
                            researchScale = 1;
                            break;
                        case "Cainhurst":
                            researchScale = 12;
                            break;
                        case "Forbidden Woods":
                            researchScale = 9;
                            break;
                        case "Byrgenwerth + Lecture hall":
                            researchScale = 11;
                            break;
                        case "Hunter's Nightmare":
                            researchScale = 24;
                            break;
                        case "Fishing Hamlet":
                            researchScale = 29;
                            break;
                        default:
                            break;
                    }
                    //public string oldIndex;
                    switch (oldIndex)
                    {
                        case "Hunter's Dream":
                            oldScale = 20;
                            break;
                        case "Hemwick Charnel Lane":
                            oldScale = 7;
                            break;
                        case "Cathedral Ward":
                            oldScale = 21;
                            break;
                        case "Upper Cathedral Ward":
                            oldScale = 14;
                            break;
                        case "Nightmare of Mensis":
                            oldScale = 17;
                            break;
                        case "Yahargul":
                            oldScale = 13;
                            break;
                        case "Nightmare Frontier":
                            oldScale = 10;
                            break;
                        case "Research Hall":
                            oldScale = 27;
                            break;
                        case "Old Yharnam":
                            oldScale = 4;
                            break;
                        case "Central Yharnam":
                            oldScale = 1;
                            break;
                        case "Cainhurst":
                            oldScale = 12;
                            break;
                        case "Forbidden Woods":
                            oldScale = 9;
                            break;
                        case "Byrgenwerth + Lecture hall":
                            oldScale = 11;
                            break;
                        case "Hunter's Nightmare":
                            oldScale = 24;
                            break;
                        case "Fishing Hamlet":
                            oldScale = 29;
                            break;
                        default:
                            break;
                    }
                    //public string centralIndex;
                    switch (centralIndex)
                    {
                        case "Hunter's Dream":
                            centralScale = 20;
                            break;
                        case "Hemwick Charnel Lane":
                            centralScale = 7;
                            break;
                        case "Cathedral Ward":
                            centralScale = 21;
                            break;
                        case "Upper Cathedral Ward":
                            centralScale = 14;
                            break;
                        case "Nightmare of Mensis":
                            centralScale = 17;
                            break;
                        case "Yahargul":
                            centralScale = 13;
                            break;
                        case "Nightmare Frontier":
                            centralScale = 10;
                            break;
                        case "Research Hall":
                            centralScale = 27;
                            break;
                        case "Old Yharnam":
                            centralScale = 4;
                            break;
                        case "Central Yharnam":
                            centralScale = 1;
                            break;
                        case "Cainhurst":
                            centralScale = 12;
                            break;
                        case "Forbidden Woods":
                            centralScale = 9;
                            break;
                        case "Byrgenwerth + Lecture hall":
                            centralScale = 11;
                            break;
                        case "Hunter's Nightmare":
                            centralScale = 24;
                            break;
                        case "Fishing Hamlet":
                            centralScale = 29;
                            break;
                        default:
                            break;
                    }
                    //public string cainhurstIndex;
                    switch (cainhurstIndex)
                    {
                        case "Hunter's Dream":
                            cainhurstScale = 20;
                            break;
                        case "Hemwick Charnel Lane":
                            cainhurstScale = 7;
                            break;
                        case "Cathedral Ward":
                            cainhurstScale = 21;
                            break;
                        case "Upper Cathedral Ward":
                            cainhurstScale = 14;
                            break;
                        case "Nightmare of Mensis":
                            cainhurstScale = 17;
                            break;
                        case "Yahargul":
                            cainhurstScale = 13;
                            break;
                        case "Nightmare Frontier":
                            cainhurstScale = 10;
                            break;
                        case "Research Hall":
                            cainhurstScale = 27;
                            break;
                        case "Old Yharnam":
                            cainhurstScale = 4;
                            break;
                        case "Central Yharnam":
                            cainhurstScale = 1;
                            break;
                        case "Cainhurst":
                            cainhurstScale = 12;
                            break;
                        case "Forbidden Woods":
                            cainhurstScale = 9;
                            break;
                        case "Byrgenwerth + Lecture hall":
                            cainhurstScale = 11;
                            break;
                        case "Hunter's Nightmare":
                            cainhurstScale = 24;
                            break;
                        case "Fishing Hamlet":
                            cainhurstScale = 29;
                            break;
                        default:
                            break;
                    }
                    //public string woodsIndex;
                    switch (woodsIndex)
                    {
                        case "Hunter's Dream":
                            woodsScale = 20;
                            break;
                        case "Hemwick Charnel Lane":
                            woodsScale = 7;
                            break;
                        case "Cathedral Ward":
                            woodsScale = 21;
                            break;
                        case "Upper Cathedral Ward":
                            woodsScale = 14;
                            break;
                        case "Nightmare of Mensis":
                            woodsScale = 17;
                            break;
                        case "Yahargul":
                            woodsScale = 13;
                            break;
                        case "Nightmare Frontier":
                            woodsScale = 10;
                            break;
                        case "Research Hall":
                            woodsScale = 27;
                            break;
                        case "Old Yharnam":
                            woodsScale = 4;
                            break;
                        case "Central Yharnam":
                            woodsScale = 1;
                            break;
                        case "Cainhurst":
                            woodsScale = 12;
                            break;
                        case "Forbidden Woods":
                            woodsScale = 9;
                            break;
                        case "Byrgenwerth + Lecture hall":
                            woodsScale = 11;
                            break;
                        case "Hunter's Nightmare":
                            woodsScale = 24;
                            break;
                        case "Fishing Hamlet":
                            woodsScale = 29;
                            break;
                        default:
                            break;
                    }
                    //public string byrgenwerthIndex;
                    switch (byrgenwerthIndex)
                    {
                        case "Hunter's Dream":
                            byrgenwerthScale = 20;
                            break;
                        case "Hemwick Charnel Lane":
                            byrgenwerthScale = 7;
                            break;
                        case "Cathedral Ward":
                            byrgenwerthScale = 21;
                            break;
                        case "Upper Cathedral Ward":
                            byrgenwerthScale = 14;
                            break;
                        case "Nightmare of Mensis":
                            byrgenwerthScale = 17;
                            break;
                        case "Yahargul":
                            byrgenwerthScale = 13;
                            break;
                        case "Nightmare Frontier":
                            byrgenwerthScale = 10;
                            break;
                        case "Research Hall":
                            byrgenwerthScale = 27;
                            break;
                        case "Old Yharnam":
                            byrgenwerthScale = 4;
                            break;
                        case "Central Yharnam":
                            byrgenwerthScale = 1;
                            break;
                        case "Cainhurst":
                            byrgenwerthScale = 12;
                            break;
                        case "Forbidden Woods":
                            byrgenwerthScale = 9;
                            break;
                        case "Byrgenwerth + Lecture hall":
                            byrgenwerthScale = 11;
                            break;
                        case "Hunter's Nightmare":
                            byrgenwerthScale = 24;
                            break;
                        case "Fishing Hamlet":
                            byrgenwerthScale = 29;
                            break;
                        default:
                            break;
                    }
                    //public string huntersNightmareIndex;
                    switch (huntersNightmareIndex)
                    {
                        case "Hunter's Dream":
                            nightmareScale = 20;
                            break;
                        case "Hemwick Charnel Lane":
                            nightmareScale = 7;
                            break;
                        case "Cathedral Ward":
                            nightmareScale = 21;
                            break;
                        case "Upper Cathedral Ward":
                            nightmareScale = 14;
                            break;
                        case "Nightmare of Mensis":
                            nightmareScale = 17;
                            break;
                        case "Yahargul":
                            nightmareScale = 13;
                            break;
                        case "Nightmare Frontier":
                            nightmareScale = 10;
                            break;
                        case "Research Hall":
                            nightmareScale = 27;
                            break;
                        case "Old Yharnam":
                            nightmareScale = 4;
                            break;
                        case "Central Yharnam":
                            nightmareScale = 1;
                            break;
                        case "Cainhurst":
                            nightmareScale = 12;
                            break;
                        case "Forbidden Woods":
                            nightmareScale = 9;
                            break;
                        case "Byrgenwerth + Lecture hall":
                            nightmareScale = 11;
                            break;
                        case "Hunter's Nightmare":
                            nightmareScale = 24;
                            break;
                        case "Fishing Hamlet":
                            nightmareScale = 29;
                            break;
                        default:
                            break;
                    }
                    //public string hamletIndex;
                    switch (hamletIndex)
                    {
                        case "Hunter's Dream":
                            hamletScale = 20;
                            break;
                        case "Hemwick Charnel Lane":
                            hamletScale = 7;
                            break;
                        case "Cathedral Ward":
                            hamletScale = 21;
                            break;
                        case "Upper Cathedral Ward":
                            hamletScale = 14;
                            break;
                        case "Nightmare of Mensis":
                            hamletScale = 17;
                            break;
                        case "Yahargul":
                            hamletScale = 13;
                            break;
                        case "Nightmare Frontier":
                            hamletScale = 10;
                            break;
                        case "Research Hall":
                            hamletScale = 27;
                            break;
                        case "Old Yharnam":
                            hamletScale = 4;
                            break;
                        case "Central Yharnam":
                            hamletScale = 1;
                            break;
                        case "Cainhurst":
                            hamletScale = 12;
                            break;
                        case "Forbidden Woods":
                            hamletScale = 9;
                            break;
                        case "Byrgenwerth + Lecture hall":
                            hamletScale = 11;
                            break;
                        case "Hunter's Nightmare":
                            hamletScale = 24;
                            break;
                        case "Fishing Hamlet":
                            hamletScale = 29;
                            break;
                        default:
                            break;
                    }
                }

                ParamScalingForBosses(filePath + "\\map\\mapstudio\\" + "m21_00_00_00.msb.dcx");
                ParamScalingForBosses(filePath + "\\map\\mapstudio\\" + "m21_01_00_00.msb.dcx");
                ParamScalingForBosses(filePath + "\\map\\mapstudio\\" + "m22_00_00_00.msb.dcx");
                ParamScalingForBosses(filePath + "\\map\\mapstudio\\" + "m23_00_00_01.msb.dcx");
                ParamScalingForBosses(filePath + "\\map\\mapstudio\\" + "m24_00_00_01.msb.dcx");
                ParamScalingForBosses(filePath + "\\map\\mapstudio\\" + "m24_01_00_01.msb.dcx");
                ParamScalingForBosses(filePath + "\\map\\mapstudio\\" + "m24_02_00_01.msb.dcx");
                ParamScalingForBosses(filePath + "\\map\\mapstudio\\" + "m25_00_00_00.msb.dcx");
                ParamScalingForBosses(filePath + "\\map\\mapstudio\\" + "m26_00_00_00.msb.dcx");
                ParamScalingForBosses(filePath + "\\map\\mapstudio\\" + "m27_00_00_01.msb.dcx");
                ParamScalingForBosses(filePath + "\\map\\mapstudio\\" + "m28_00_00_01.msb.dcx");
                ParamScalingForBosses(filePath + "\\map\\mapstudio\\" + "m32_00_00_01.msb.dcx");
                ParamScalingForBosses(filePath + "\\map\\mapstudio\\" + "m33_00_00_00.msb.dcx");
                ParamScalingForBosses(filePath + "\\map\\mapstudio\\" + "m34_00_00_00.msb.dcx");
                ParamScalingForBosses(filePath + "\\map\\mapstudio\\" + "m35_00_00_00.msb.dcx");
                ParamScalingForBosses(filePath + "\\map\\mapstudio\\" + "m36_00_00_00.msb.dcx");
                ParamScalingForBosses(filePath + "\\map\\mapstudio\\" + "m29_10_90_00\\m29_10_90_00.msb.dcx");
                ParamScalingForBosses(filePath + "\\map\\mapstudio\\" + "m29_10_90_00\\m29_10_90_01.msb.dcx");
                ParamScalingForBosses(filePath + "\\map\\mapstudio\\" + "m29_20_90_00\\m29_20_90_00.msb.dcx");
                ParamScalingForBosses(filePath + "\\map\\mapstudio\\" + "m29_20_90_00\\m29_20_90_01.msb.dcx");
                ParamScalingForBosses(filePath + "\\map\\mapstudio\\" + "m29_21_90_00\\m29_21_90_00.msb.dcx");
                ParamScalingForBosses(filePath + "\\map\\mapstudio\\" + "m29_30_90_00\\m29_30_90_00.msb.dcx");
                ParamScalingForBosses(filePath + "\\map\\mapstudio\\" + "m29_30_90_00\\m29_30_90_01.msb.dcx");
                ParamScalingForBosses(filePath + "\\map\\mapstudio\\" + "m29_31_90_00\\m29_31_90_00.msb.dcx");
                ParamScalingForBosses(filePath + "\\map\\mapstudio\\" + "m29_40_90_00\\m29_40_90_00.msb.dcx");
                ParamScalingForBosses(filePath + "\\map\\mapstudio\\" + "m29_40_90_00\\m29_40_90_01.msb.dcx");
                ParamScalingForBosses(filePath + "\\map\\mapstudio\\" + "m29_42_90_00\\m29_42_90_00.msb.dcx");
                ParamScalingForBosses(filePath + "\\map\\mapstudio\\" + "m29_42_90_00\\m29_42_90_01.msb.dcx");
                ParamScalingForBosses(filePath + "\\map\\mapstudio\\" + "m29_50_90_00\\m29_50_90_00.msb.dcx");
                ParamScalingForBosses(filePath + "\\map\\mapstudio\\" + "m29_50_90_00\\m29_50_90_01.msb.dcx");
                ParamScalingForBosses(filePath + "\\map\\mapstudio\\" + "m29_52_90_00\\m29_52_90_00.msb.dcx");
                ParamScalingForBosses(filePath + "\\map\\mapstudio\\" + "m29_52_90_00\\m29_52_90_01.msb.dcx");
                ParamScalingForBosses(filePath + "\\map\\mapstudio\\" + "m29_53_90_00\\m29_53_90_00.msb.dcx");
                ParamScalingForBosses(filePath + "\\map\\mapstudio\\" + "m29_53_90_00\\m29_53_90_01.msb.dcx");
                if (GOBAdded)
                {
                    ParamScalingForBosses(filePath + "\\map\\mapstudio\\" + "m29_50_40_00\\m29_50_40_00.msb.dcx");
                }
            }
            mapList = new List<string>();

            mapList.Add(filePath + "\\map\\mapstudio\\" + "m21_00_00_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m21_01_00_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m22_00_00_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m23_00_00_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m23_00_00_01.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m24_00_00_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m24_00_00_01.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m24_01_00_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m24_01_00_01.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m24_01_00_11.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m24_02_00_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m24_02_00_01.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m25_00_00_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m26_00_00_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m27_00_00_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m27_00_00_01.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m28_00_00_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m28_00_00_01.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m32_00_00_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m32_00_00_01.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m33_00_00_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m34_00_00_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m35_00_00_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m36_00_00_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_10_90_00\\m29_10_90_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_10_90_00\\m29_10_90_01.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_20_90_00\\m29_20_90_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_20_90_00\\m29_20_90_01.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_21_90_00\\m29_21_90_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_30_90_00\\m29_30_90_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_30_90_00\\m29_30_90_01.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_31_90_00\\m29_31_90_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_40_90_00\\m29_40_90_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_40_90_00\\m29_40_90_01.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_42_90_00\\m29_42_90_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_42_90_00\\m29_42_90_01.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_50_90_00\\m29_50_90_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_50_90_00\\m29_50_90_01.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_52_90_00\\m29_52_90_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_52_90_00\\m29_52_90_01.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_53_90_00\\m29_53_90_00.msb.dcx");
            mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_53_90_00\\m29_53_90_01.msb.dcx");

            if (GOBAdded)
            {
                mapList.Add(filePath + "\\map\\mapstudio\\" + "m29_50_40_00\\m29_50_40_00.msb.dcx");
            }

            for (int i = 0; i < mapList.Count; i++)
            {
                var tempMapRead = MSBB.Read(mapList[i]);

                long maxSizeLong = 0;

                for (int j = 0; j < tempMapRead.Parts.Enemies.Count; j++)
                {
                    for (int k = 0; k < nameList.Count; k++)
                    {
                        if (tempMapRead.Parts.Enemies[j].ModelName.Contains(nameList[k]))
                        {
                            maxSizeLong += sizeList[k];
                            k = nameList.Count + 1;
                        }
                    }
                }
            }

            this.Dispatcher.Invoke(() =>
            {
                totalPercent = 100;
                currentTask = "FINISHED!";
                UpdateUI();
            });


            running = false;

            MessageBox.Show($"Finished.  Seed: {seed}.");

            this.Dispatcher.Invoke(() =>
            {
                currentTask = "CURRENT TASK";
                totalPercent = 0;
                UpdateUI();
                totalTime = 0;
                TotalTime.Content = "0:00";

                TEST.IsEnabled = true;
                RandomizeEnemiesCheck.IsEnabled = true;
                BossCheckBox.IsEnabled = true;
                InsertBosses.IsEnabled = true;
                ChaliceBoss.IsEnabled = true;
                ChaliceEnemies.IsEnabled = true;
                ArmorRandomizerCheckBox.IsEnabled = true;
                AddNPCS.IsEnabled = true;
                OopsAllCheck.IsEnabled = true;
                OopsAllBossesCheck.IsEnabled = true;
                OopsAllStringName.IsEnabled = true;
                OopsBoss.IsEnabled = true;
                BellMaidenBox.IsEnabled = true;
                LesserBossesBox.IsEnabled = true;
                RandomizeKeyItemsBox.IsEnabled = true;
                RandomizeShopBox.IsEnabled = true;
                EnemyDropBox.IsEnabled = true;
                WorkshopBox.IsEnabled = true;
            });

            Environment.Exit(0);
        }
    }
}
