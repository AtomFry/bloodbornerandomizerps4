using SoulsFormats;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace MSB_Test
{
    public partial class MainWindow
    {
        private void Randomize(string currentMap, List<string> nonoList)
        {
            if (randomize)
            {
                var tempGUY = MSBB.Read(currentMap);

                bool changeData;
                bool cc = true;

                if (!oopsAll && randomizeEnemiesBool)
                {
                    for (int i = 0; i < tempGUY.Parts.Enemies.Count; i++)
                    {
                        changeData = true;

                        for (int j = 0; j < nonoList.Count; j++)
                        {
                            if (tempGUY.Parts.Enemies[i].Name.Contains(nonoList[j]))
                            {
                                changeData = false;
                                cc = false;
                            }
                        }

                        if (currentMap.Contains("m29"))
                        {
                            for (int j = 0; j < chaliceBossParams.Count; j++)
                            {
                                if (tempGUY.Parts.Enemies[i].NPCParamID.ToString() == chaliceBossParams[j])
                                {
                                    changeData = false;
                                }

                                if (tempGUY.Parts.Enemies[i].ModelName.Contains("1050"))
                                {
                                    changeData = false;
                                }
                            }
                        }

                        if (currentMap.Contains("22_00"))
                        {

                            int randChoose = universalRand.Next(0, 101);
                            if (randChoose >= HemwickChance)
                            {
                                changeData = false;
                            }
                            else
                            {
                                if (cc)
                                {
                                    changeData = true;
                                }
                            }
                        }

                        if (currentMap.Contains("23_00"))
                        {

                            int randChoose = universalRand.Next(0, 101);
                            if (randChoose >= OldYharnamChance)
                            {
                                changeData = false;
                            }
                            else
                            {
                                if (cc)
                                {
                                    changeData = true;
                                }
                            }
                        }

                        if (currentMap.Contains("24_00"))
                        {

                            int randChoose = universalRand.Next(0, 101);
                            if (randChoose >= CathedralWardChance)
                            {
                                changeData = false;
                            }
                            else
                            {
                                if (cc)
                                {
                                    changeData = true;
                                }
                            }
                        }

                        if (currentMap.Contains("24_01"))
                        {

                            int randChoose = universalRand.Next(0, 101);
                            if (randChoose >= CentralYharnamChance)
                            {
                                changeData = false;
                            }
                            else
                            {
                                if (cc)
                                {
                                    changeData = true;
                                }
                            }
                        }

                        if (currentMap.Contains("24_02"))
                        {

                            int randChoose = universalRand.Next(0, 101);
                            if (randChoose >= UpperCathedralWardChance)
                            {
                                changeData = false;
                            }
                            else
                            {
                                if (cc)
                                {
                                    changeData = true;
                                }
                            }
                        }

                        if (currentMap.Contains("25_00"))
                        {

                            int randChoose = universalRand.Next(0, 101);
                            if (randChoose >= CainhurstChance)
                            {
                                changeData = false;
                            }
                            else
                            {
                                if (cc)
                                {
                                    changeData = true;
                                }
                            }
                        }

                        if (currentMap.Contains("26_00"))
                        {

                            int randChoose = universalRand.Next(0, 101);
                            if (randChoose >= NightmareOfMensisChance)
                            {
                                changeData = false;
                            }
                            else
                            {
                                if (cc)
                                {
                                    changeData = true;
                                }
                            }
                        }

                        if (currentMap.Contains("27_00"))
                        {

                            int randChoose = universalRand.Next(0, 101);
                            if (randChoose >= WoodsChance)
                            {
                                changeData = false;
                            }
                            else
                            {
                                if (cc)
                                {
                                    changeData = true;
                                }
                            }
                        }

                        if (currentMap.Contains("28_00"))
                        {

                            int randChoose = universalRand.Next(0, 101);
                            if (randChoose >= YahargulChance)
                            {
                                changeData = false;
                            }
                            else
                            {
                                if (cc)
                                {
                                    changeData = true;
                                }
                            }
                        }

                        if (currentMap.Contains("32_00"))
                        {

                            int randChoose = universalRand.Next(0, 101);
                            if (randChoose >= ByrgenwerthChance)
                            {
                                changeData = false;
                            }
                            else
                            {
                                if (cc)
                                {
                                    changeData = true;
                                }
                            }
                        }

                        if (currentMap.Contains("33_00"))
                        {

                            int randChoose = universalRand.Next(0, 101);
                            if (randChoose >= FrontierChance)
                            {
                                changeData = false;
                            }
                            else
                            {
                                if (cc)
                                {
                                    changeData = true;
                                }
                            }
                        }

                        if (currentMap.Contains("34_00"))
                        {

                            int randChoose = universalRand.Next(0, 101);
                            if (randChoose >= HuntersNightmareChance)
                            {
                                changeData = false;
                            }
                            else
                            {
                                if (cc)
                                {
                                    changeData = true;
                                }
                            }
                        }

                        if (currentMap.Contains("35_00"))
                        {

                            int randChoose = universalRand.Next(0, 101);
                            if (randChoose >= ResearchHallChance)
                            {
                                changeData = false;
                            }
                            else
                            {
                                if (cc)
                                {
                                    changeData = true;
                                }
                            }
                        }

                        if (currentMap.Contains("36_00"))
                        {

                            int randChoose = universalRand.Next(0, 101);
                            if (randChoose >= HamletChance)
                            {
                                changeData = false;
                            }
                            else
                            {
                                if (cc)
                                {
                                    changeData = true;
                                }
                            }
                        }

                        if (currentMap.Contains("m28"))
                        {
                            if (tempGUY.Parts.Enemies[i].Name.Contains("c1050_0117"))
                            {
                                changeData = true;
                            }

                            if (tempGUY.Parts.Enemies[i].Name.Contains("c1050_0115"))
                            {
                                changeData = true;
                            }

                            if (tempGUY.Parts.Enemies[i].Name.Contains("c1050_0119"))
                            {
                                changeData = true;
                            }

                            if (tempGUY.Parts.Enemies[i].Name.Contains("c1050_0110"))
                            {
                                changeData = true;
                            }

                            if (tempGUY.Parts.Enemies[i].Name.Contains("c1050_0112"))
                            {
                                changeData = true;
                            }

                            if (tempGUY.Parts.Enemies[i].Name.Contains("c1050_0114"))
                            {
                                changeData = true;
                            }
                        }

                        if (changeData && enemyDataRandomized.Count > 0)
                        {

                            float chaliceRandom = universalRand.Next(0, 101);
                            int randomChalice = 0;
                            if (chaliceEnemies)
                            {
                                randomChalice = universalRand.Next(0, chaliceEnemiesString.Count);
                            }
                            int random = universalRand.Next(0, enemyDataRandomized.Count);
                            string thisEnemy;
                            if (chaliceRandom <= chaliceChanceFloat && chaliceEnemies)
                            {
                                thisEnemy = chaliceEnemiesString[random];
                            }
                            else if (currentMap.Contains("m29") && chaliceEnemies)
                            {
                                thisEnemy = chaliceEnemiesString[randomChalice];
                            }
                            else
                            {
                                thisEnemy = enemyDataRandomized[random];
                            }

                            string tempNpcParam;
                            string tempThinkId;
                            string modelName;

                            int tempNpcParamInt;
                            int tempThinkIdInt;

                            Console.WriteLine(enemyDataRandomized[0]);

                            int tempnpcint = thisEnemy.IndexOf("*");
                            tempNpcParam = thisEnemy.Substring(0, tempnpcint);
                            tempThinkId = thisEnemy.Substring(tempnpcint + 1, thisEnemy.LastIndexOf("*") - tempnpcint - 1);
                            modelName = thisEnemy.Substring(thisEnemy.LastIndexOf("*") + 1, 5);

                            string originalModelName = tempGUY.Parts.Enemies[i].ModelName;
                            long originalModelValue = 0;
                            long newModelValue = 0;

                            int tries = 0;

                            for (int j = 0; j < nameList.Count; j++)
                            {
                                if (nameList[j].Contains(originalModelName))
                                {
                                    originalModelValue = sizeList[j];
                                }

                                if (nameList[j].Contains(modelName))
                                {
                                    newModelValue = sizeList[j];
                                }
                            }

                            if (currentMap.Contains("m24_02") || currentMap.Contains("m35"))
                            {
                                while (newModelValue > (originalModelValue) && tries < 30)
                                {
                                    if (chaliceEnemies)
                                    {
                                        randomChalice = universalRand.Next(0, chaliceEnemiesString.Count);
                                    }
                                    random = universalRand.Next(0, enemyDataRandomized.Count);
                                    if (chaliceRandom <= chaliceChanceFloat && chaliceEnemies)
                                    {
                                        thisEnemy = chaliceEnemiesString[random];
                                    }
                                    else if (currentMap.Contains("m29") && chaliceEnemies)
                                    {
                                        thisEnemy = chaliceEnemiesString[randomChalice];
                                    }
                                    else
                                    {
                                        thisEnemy = enemyDataRandomized[random];
                                    }

                                    tempnpcint = thisEnemy.IndexOf("*");
                                    tempNpcParam = thisEnemy.Substring(0, tempnpcint);
                                    tempThinkId = thisEnemy.Substring(tempnpcint + 1, thisEnemy.LastIndexOf("*") - tempnpcint - 1);
                                    modelName = thisEnemy.Substring(thisEnemy.LastIndexOf("*") + 1, 5);

                                    for (int j = 0; j < nameList.Count; j++)
                                    {
                                        if (nameList[j].Contains(originalModelName))
                                        {
                                            originalModelValue = sizeList[j];
                                        }

                                        if (nameList[j].Contains(modelName))
                                        {
                                            newModelValue = sizeList[j];

                                        }
                                    }

                                    tries++;
                                }
                            }
                            else
                            {
                                while (newModelValue > (originalModelValue * sizeOfEnemy) && tries < 30000)
                                {
                                    if (chaliceEnemies)
                                    {
                                        randomChalice = universalRand.Next(0, chaliceEnemiesString.Count);
                                    }
                                    random = universalRand.Next(0, enemyDataRandomized.Count);
                                    if (chaliceRandom <= chaliceChanceFloat && chaliceEnemies)
                                    {
                                        thisEnemy = chaliceEnemiesString[random];
                                    }
                                    else if (currentMap.Contains("m29") && chaliceEnemies)
                                    {
                                        thisEnemy = chaliceEnemiesString[randomChalice];
                                    }
                                    else
                                    {
                                        thisEnemy = enemyDataRandomized[random];
                                    }

                                    tempnpcint = thisEnemy.IndexOf("*");
                                    tempNpcParam = thisEnemy.Substring(0, tempnpcint);
                                    tempThinkId = thisEnemy.Substring(tempnpcint + 1, thisEnemy.LastIndexOf("*") - tempnpcint - 1);
                                    modelName = thisEnemy.Substring(thisEnemy.LastIndexOf("*") + 1, 5);

                                    for (int j = 0; j < nameList.Count; j++)
                                    {
                                        if (nameList[j].Contains(originalModelName))
                                        {
                                            originalModelValue = sizeList[j];
                                        }

                                        if (nameList[j].Contains(modelName))
                                        {
                                            newModelValue = sizeList[j];

                                        }
                                    }

                                    tries++;
                                }
                            }

                            if (currentMap.Contains("m24_02"))
                            {
                                while (modelName.Contains("c1000") || modelName.Contains("c5040") || modelName.Contains("2630")
                                    || modelName.Contains("1051") || modelName.Contains("c1180"))
                                {
                                    random = universalRand.Next(0, enemyDataRandomized.Count);
                                    thisEnemy = enemyDataRandomized[random];
                                    tempnpcint = thisEnemy.IndexOf("*");
                                    tempNpcParam = thisEnemy.Substring(0, tempnpcint);
                                    tempThinkId = thisEnemy.Substring(tempnpcint + 1, thisEnemy.LastIndexOf("*") - tempnpcint - 1);
                                    modelName = thisEnemy.Substring(thisEnemy.LastIndexOf("*") + 1, 5);
                                }
                            }

                            if (currentMap.Contains("m35"))
                            {
                                while (modelName.Contains("c1000") || modelName.Contains("c5040") || modelName.Contains("2630")
                                    || modelName.Contains("1051") || modelName.Contains("c1180") || modelName.Contains("c1111")
                                     || modelName.Contains("c2620") || modelName.Contains("2090") || modelName.Contains("1250")
                                      || modelName.Contains("1260"))
                                {
                                    random = universalRand.Next(0, enemyDataRandomized.Count);
                                    thisEnemy = enemyDataRandomized[random];
                                    tempnpcint = thisEnemy.IndexOf("*");
                                    tempNpcParam = thisEnemy.Substring(0, tempnpcint);
                                    tempThinkId = thisEnemy.Substring(tempnpcint + 1, thisEnemy.LastIndexOf("*") - tempnpcint - 1);
                                    modelName = thisEnemy.Substring(thisEnemy.LastIndexOf("*") + 1, 5);
                                }
                            }


                            tempNpcParamInt = int.Parse(tempNpcParam);
                            tempThinkIdInt = int.Parse(tempThinkId);

                            string npc = tempGUY.Parts.Enemies[i].NPCParamID.ToString();
                            string think = tempGUY.Parts.Enemies[i].ThinkParamID.ToString();
                            string mo = tempGUY.Parts.Enemies[i].ModelName;
                            string entityID = tempGUY.Parts.Enemies[i].EntityID.ToString();

                            tempGUY.Parts.Enemies[i].NPCParamID = tempNpcParamInt;
                            tempGUY.Parts.Enemies[i].ThinkParamID = tempThinkIdInt;
                            tempGUY.Parts.Enemies[i].ModelName = modelName;

                            if (logging)
                                using (StreamWriter writetext = File.AppendText(enemyLogFilePath))
                                {
                                    writetext.WriteLine(currentMap);
                                    writetext.WriteLine(i + " Number in map list");
                                    writetext.WriteLine("Old Enemy");
                                    writetext.WriteLine(npc + " npcParam");
                                    writetext.WriteLine(think + " thinkID");
                                    writetext.WriteLine(mo + " model");
                                    writetext.WriteLine(entityID);

                                    writetext.WriteLine("New Enemy");
                                    writetext.WriteLine(tempNpcParam + " npcParam");
                                    writetext.WriteLine(tempThinkId + " thinkID");
                                    writetext.WriteLine(modelName + " new model" + Environment.NewLine);
                                }
                        }


                    }
                }
                else if (oopsAll)
                {
                    for (int i = 0; i < tempGUY.Parts.Enemies.Count; i++)
                    {
                        changeData = true;

                        for (int j = 0; j < unusedList.Count; j++)
                        {
                            if (tempGUY.Parts.Enemies[i].Name.Contains(nonoList[j]))
                            {
                                changeData = false;
                            }
                        }

                        if (changeData && oopsAllEnemyString.Count > 0)
                        {

                            int random = universalRand.Next(0, oopsAllEnemyString.Count);
                            string thisEnemy;
                            thisEnemy = oopsAllEnemyString[random];

                            string tempNpcParam;
                            string tempThinkId;
                            string modelName;

                            int tempNpcParamInt;
                            int tempThinkIdInt;

                            Console.WriteLine(oopsAllEnemyString[0]);

                            int tempnpcint = thisEnemy.IndexOf("*");
                            tempNpcParam = thisEnemy.Substring(0, tempnpcint);
                            tempThinkId = thisEnemy.Substring(tempnpcint + 1, thisEnemy.LastIndexOf("*") - tempnpcint - 1);
                            modelName = thisEnemy.Substring(thisEnemy.LastIndexOf("*") + 1, 5);

                            tempNpcParamInt = int.Parse(tempNpcParam);
                            tempThinkIdInt = int.Parse(tempThinkId);

                            string npc = tempGUY.Parts.Enemies[i].NPCParamID.ToString();
                            string think = tempGUY.Parts.Enemies[i].ThinkParamID.ToString();
                            string mo = tempGUY.Parts.Enemies[i].ModelName;
                            string entityID = tempGUY.Parts.Enemies[i].EntityID.ToString();

                            tempGUY.Parts.Enemies[i].NPCParamID = tempNpcParamInt;
                            tempGUY.Parts.Enemies[i].ThinkParamID = tempThinkIdInt;
                            tempGUY.Parts.Enemies[i].ModelName = modelName;

                            if (logging)
                                using (StreamWriter writetext = File.AppendText(enemyLogFilePath))
                                {
                                    writetext.WriteLine(currentMap);
                                    writetext.WriteLine(i + " Number in map list");
                                    writetext.WriteLine("Old Enemy");
                                    writetext.WriteLine(npc + " npcParam");
                                    writetext.WriteLine(think + " thinkID");
                                    writetext.WriteLine(mo + " model");
                                    writetext.WriteLine(entityID);

                                    writetext.WriteLine("New Enemy");
                                    writetext.WriteLine(tempNpcParam + " npcParam");
                                    writetext.WriteLine(tempThinkId + " thinkID");
                                    writetext.WriteLine(modelName + " new model" + Environment.NewLine);
                                }
                        }


                    }
                }
                tempGUY.Write(currentMap);
            }
        }

        private void RandomizeStartingWeapons()
        {

        }

        private void RandomizeShopItems()
        {
            var paramdefs = new List<PARAMDEF>();
            var paramdefbnd = BND4.Read(paramDefPath);
            foreach (BinderFile file in paramdefbnd.Files)
            {
                var paramdef = PARAMDEF.Read(file.Bytes);
                paramdefs.Add(paramdef);
            }

            var parms = new Dictionary<string, PARAM>();
            var parambnd = BND4.Read(paramPath);
            foreach (BinderFile file in parambnd.Files)
            {
                string name = Path.GetFileNameWithoutExtension(file.Name);
                var param = PARAM.Read(file.Bytes);

                param.ApplyParamdef(paramdefs.Find(def => def.ParamType == param.ParamType));
                parms[name] = param;
            }

            PARAM shopLineupParams = parms["ShopLineupParam"];
            PARAM weaponParams = parms["EquipParamWeapon"];



            List<string> vialAndBulletIDsList = new List<string>();
            vialAndBulletIDsList.Add("1000");
            vialAndBulletIDsList.Add("900");
            vialAndBulletIDsList.Add("240");
            if (keepGuns)
            {
                vialAndBulletIDsList.Add("14000000");
                vialAndBulletIDsList.Add("6000000");
            }

            List<string> shopWeaponList = new List<string>();
            List<string> shopConsumableList = new List<string>();
            List<string> shopArmorList = new List<string>();

            //EQUIP ID'S

            //RIGHT HAND WEAPONS
            //Parasite 38000000
            //Amygdalan Arm? 25000000
            //ax 5000000
            //cane 22000000
            //lost blood sword 2020000
            //chikage 2000000
            //hammer 8000000
            //war pick? 30000000
            //lost war pick? 30020000
            //lost hammer 8020000
            //cut off the lost meat??? 24020000
            //double edged decidious??? 27000000
            //gun hammer? 28000000
            //funeral blade 5100000
            //gatling gun 33000000
            //chigatana heterogenity 2010000
            //Heterogeneity of church stone mallet 8010000
            //Heterogeneity of deciduous 27010000
            //Heterogeneity of Funeral blade 5110000
            //Heterogeneity of Juyari 10010000
            //Heterogeneity of Leyte Rupa rush 10110000
            //Heterogeneity of mercy blade 4010000
            //Heterogeneity of moonlight Seiken 26010000
            //Heterogeneity of parasite 38010000
            //Heterogeneity of phlebotomy mallet 29010000
            //Heterogeneity of rotation sawtooth 31010000
            //Heterogeneity of Shishitsume 9010000
            //Heterogeneity of small Amun arm 25010000
            //Heterogeneity of the beast hunting ax 5010000
            //Heterogeneity of the beast hunting songs sword 23010000
            //Heterogeneity of the bow sword 32010000
            //Heterogeneity of the church pile 30010000
            //Heterogeneity of the explosion hammer 28010000
            //Heterogeneity of the feed wand 22010000
            //Heterogeneity of the holy sword 8110000
            //Heterogeneity of the meat cut off 24010000
            //Heterogeneity of the pile hammer 11010000
            //Heterogeneity of the sawtooth hatchet 7010000
            //Heterogeneity of the wheels 12010000
            //Heterogeneity of Tonitorusu 13010000
            //Holy sword of moonlight (moonlight sword) 26000000
            //Holy sword of Rudouiku (sword sword) 8100000
            //Juyari (Juyari) 10000000
            //Juyari lost 10020000
            //Leyte Rupa Rush (bayonet) 10100000
            //Lost beast hunting ax 5020000
            //Lost beast hunting songs sword 23020000
            //Lost bow sword 32020000
            //Lost charged cane 22020000
            //Lost deciduous 27020000
            //Lost explosion hammer 28020000
            //Lost Funeral blade 5120000
            //Lost Leyte Rupa rush 10120000
            //Lost mercy blade 4020000
            //Lost moonlight Seiken 26020000
            //Lost parasites 38020000
            //Lost pile hammer 11020000
            //Lost wheel 12020000
            //Meat cut off (Nokomuchi) 24000000
            //Mercy of the blade (twin sword) 4000000
            //Nokonata 7000000
            //Phlebotomy hammer lost 29020000
            //Phlebotomy of the mallet (cross Mace) 29000000
            //Pile hammer (Kenkui) 11000000
            //Pole ax 5000000
            //Rogeriusu of the wheel (wheel) 12000000
            //Rotation sawtooth (circular saw chain saw) 31000000
            //Rotation sawtooth lost 31020000
            //Sawtooth hatchet(Nokonata) 7000000
            //Sawtooth hatchet that lost 7020000
            //Sawtooth spear (Nokoyari) 7100000
            //Sawtooth spear lost 7120000
            //Shishitsume lost 9020000
            //Simon bow sword (deformation bow) 32000000
            //Small Amun arm that was lost 25020000
            //Song sword of the beast hunting (saw song sword) 23000000
            //St. sword that was lost 8120000
            //Stick whip 22000000
            //Tonitorusu (爆槌) 13000000
            //Tonitorusu lost 13020000

            rightHandList.Add("38000000");
            rightHandList.Add("25000000");
            rightHandList.Add("5000000");
            rightHandList.Add("22000000");
            rightHandList.Add("2020000");
            rightHandList.Add("2000000");
            rightHandList.Add("8000000");
            rightHandList.Add("30000000");
            rightHandList.Add("30020000");
            rightHandList.Add("8020000");
            rightHandList.Add("24020000");
            rightHandList.Add("27000000");
            rightHandList.Add("28000000");
            rightHandList.Add("5100000");
            rightHandList.Add("2010000");
            rightHandList.Add("8010000");
            rightHandList.Add("27010000");
            rightHandList.Add("5110000");
            rightHandList.Add("10010000");
            rightHandList.Add("10110000");
            rightHandList.Add("4010000");
            rightHandList.Add("26010000");
            rightHandList.Add("38010000");
            rightHandList.Add("29010000");
            rightHandList.Add("31010000");
            rightHandList.Add("9010000");
            rightHandList.Add("25010000");
            rightHandList.Add("5010000");
            rightHandList.Add("23010000");
            rightHandList.Add("32010000");
            rightHandList.Add("30010000");
            rightHandList.Add("28010000");
            rightHandList.Add("22010000");
            rightHandList.Add("8110000");
            rightHandList.Add("24010000");
            rightHandList.Add("11010000");
            rightHandList.Add("7010000");
            rightHandList.Add("12010000");
            rightHandList.Add("13010000");
            rightHandList.Add("26000000");
            rightHandList.Add("8100000");
            rightHandList.Add("10000000");
            rightHandList.Add("10020000");
            rightHandList.Add("10100000");
            rightHandList.Add("5020000");
            rightHandList.Add("23020000");
            rightHandList.Add("32020000");
            rightHandList.Add("22020000");
            rightHandList.Add("27020000");
            rightHandList.Add("28020000");
            rightHandList.Add("5120000");
            rightHandList.Add("10120000");
            rightHandList.Add("4020000");
            rightHandList.Add("26020000");
            rightHandList.Add("38020000");
            rightHandList.Add("11020000");
            rightHandList.Add("12020000");
            rightHandList.Add("24000000");
            rightHandList.Add("4000000");
            rightHandList.Add("7000000");
            rightHandList.Add("29020000");
            rightHandList.Add("/29000000");
            rightHandList.Add("11000000");
            rightHandList.Add("5000000");
            rightHandList.Add("12000000");
            rightHandList.Add("31000000");
            rightHandList.Add("31020000");
            rightHandList.Add("7000000");
            rightHandList.Add("7020000");
            rightHandList.Add("7100000");
            rightHandList.Add("7120000");
            rightHandList.Add("9020000");
            rightHandList.Add("32000000");
            rightHandList.Add("25020000");
            rightHandList.Add("23000000");
            rightHandList.Add("8120000");
            rightHandList.Add("22000000");
            rightHandList.Add("13000000");
            rightHandList.Add("13020000");

            //LEFT HAND WEAPONS
            //cannon 15000000
            //church cannon 35000000
            //evelyn 14100000
            //flamethrower 18100000
            //fist of gratia 34000000
            //Long gun Rudouiku (church length gun)??? 6100000
            //Of the beast hunting handguns (workshop handguns) 14000000
            //Of the beast hunting shotgun (workshop Nagaju) 6000000
            //Rosmarinus (sprayer) 18000000\
            //Shield of the lake (attribute shield) 19100000
            //Through gun (through gun) 36000000
            //Torches of the beast hunting (torches of the hunter) 20000000
            //Twin gun of the Church (Church handguns) 14200000
            //Workshop handgun 14000000
            //Workshop Nagaju 6000000

            leftHandList.Add("15000000");
            leftHandList.Add("35000000");
            leftHandList.Add("14100000");
            leftHandList.Add("18100000");
            leftHandList.Add("34000000");
            leftHandList.Add("6100000");
            leftHandList.Add("14000000");
            leftHandList.Add("6000000");
            leftHandList.Add("18000000");
            leftHandList.Add("19100000");
            leftHandList.Add("36000000");
            leftHandList.Add("20000000");
            leftHandList.Add("14200000");
            leftHandList.Add("14000000");
            leftHandList.Add("6000000");
            leftHandList.Add("33000000");

            for (int i = 0; i < shopLineupParams.Rows.Count; i++)
            {
                bool addToList = true;

                for (int j = 0; j < vialAndBulletIDsList.Count; j++)
                {
                    if (shopLineupParams.Rows[i].Cells[0].Value.ToString() == vialAndBulletIDsList[j])
                    {
                        addToList = false;
                    }
                }

                if (addToList)
                {
                    if (shopLineupParams.Rows[i].Cells[7].Value.ToString() == "0")
                    {
                        shopWeaponList.Add(shopLineupParams.Rows[i].Cells[0].Value.ToString());
                    }
                    else if (shopLineupParams.Rows[i].Cells[7].Value.ToString() == "1")
                    {
                        shopArmorList.Add(shopLineupParams.Rows[i].Cells[0].Value.ToString());
                    }
                    else if (shopLineupParams.Rows[i].Cells[7].Value.ToString() == "3")
                    {
                        shopConsumableList.Add(shopLineupParams.Rows[i].Cells[0].Value.ToString());
                    }
                }
            }

            for (int i = 0; i < shopLineupParams.Rows.Count; i++)
            {
                bool changeData = true;

                for (int j = 0; j < vialAndBulletIDsList.Count; j++)
                {
                    if (shopLineupParams.Rows[i].Cells[0].Value.ToString() == vialAndBulletIDsList[j])
                    {
                        changeData = false;
                    }
                }

                if (changeData)
                {
                    if (shopLineupParams.Rows[i].Cells[7].Value.ToString() == "0" && shopWeaponList.Count > 0)
                    {
                        if (startingWeaponsOnlyBool)
                        {
                            int randomNumber = universalRand.Next(0, shopWeaponList.Count);

                            while (shopWeaponList[randomNumber].Contains(shopLineupParams.Rows[i].Cells[0].ToString()))
                            {
                                randomNumber = universalRand.Next(0, shopWeaponList.Count);
                            }

                            if (shopLineupParams.Rows[i].Cells[0].ToString().Contains("7000000")
                                || shopLineupParams.Rows[i].Cells[0].ToString().Contains("5000000")
                                || shopLineupParams.Rows[i].Cells[0].ToString().Contains("22000000"))
                            {
                                bool rightHand = false;

                                while (!rightHand)
                                {
                                    if (!rightHandList.Contains(shopWeaponList[randomNumber]))
                                    {
                                        randomNumber = universalRand.Next(0, shopWeaponList.Count);
                                    }
                                    else
                                    {
                                        rightHand = true;
                                    }
                                }

                            }

                            if (shopLineupParams.Rows[i].Cells[0].ToString().Contains("14000000")
                                || shopLineupParams.Rows[i].Cells[0].ToString().Contains("6000000"))
                            {
                                bool leftHand = false;

                                while (!leftHand)
                                {
                                    if (!leftHandList.Contains(shopWeaponList[randomNumber]))
                                    {
                                        randomNumber = universalRand.Next(0, shopWeaponList.Count);
                                    }
                                    else
                                    {
                                        leftHand = true;
                                    }
                                }

                            }

                            if (shopLineupParams.Rows[i].ID.ToString() == "2000")
                            {
                                new2000 = shopWeaponList[randomNumber];
                            }

                            if (shopLineupParams.Rows[i].ID.ToString() == "2001")
                            {
                                new2001 = shopWeaponList[randomNumber];
                            }

                            if (shopLineupParams.Rows[i].ID.ToString() == "2002")
                            {
                                new2002 = shopWeaponList[randomNumber];
                            }

                            if (!keepGuns)
                            {
                                if (shopLineupParams.Rows[i].ID.ToString() == "2010")
                                {
                                    new2010 = shopWeaponList[randomNumber];
                                }

                                if (shopLineupParams.Rows[i].ID.ToString() == "2011")
                                {
                                    new2011 = shopWeaponList[randomNumber];
                                }
                            }

                            if (logging)
                                using (StreamWriter writetext = File.AppendText(randomizedItemLotPath))
                                {
                                    writetext.WriteLine("Old Shop Item");
                                    writetext.WriteLine(shopLineupParams.Rows[i].Cells[0].Value);
                                    writetext.WriteLine("New Shop Item");
                                    writetext.WriteLine(shopWeaponList[randomNumber]);
                                }

                            shopLineupParams.Rows[i].Cells[0].Value = Int32.Parse(shopWeaponList[randomNumber]);

                            shopWeaponList.RemoveAt(randomNumber);
                        }
                    }
                    else if (shopLineupParams.Rows[i].Cells[7].Value.ToString() == "1" && shopArmorList.Count > 0)
                    {
                        if (shopBool)
                        {
                            int randomNumber = universalRand.Next(0, shopArmorList.Count);

                            if (logging)
                                using (StreamWriter writetext = File.AppendText(randomizedItemLotPath))
                                {
                                    writetext.WriteLine("Old Shop Item");
                                    writetext.WriteLine(shopLineupParams.Rows[i].Cells[0].Value);
                                    writetext.WriteLine("New Shop Item");
                                    writetext.WriteLine(shopArmorList[randomNumber]);
                                }

                            shopLineupParams.Rows[i].Cells[0].Value = Int32.Parse(shopArmorList[randomNumber]);

                            shopArmorList.RemoveAt(randomNumber);
                        }
                    }
                    else if (shopLineupParams.Rows[i].Cells[7].Value.ToString() == "3" && shopConsumableList.Count > 0)
                    {
                        if (shopBool)
                        {
                            int randomNumber = universalRand.Next(0, shopConsumableList.Count);

                            if (logging)
                                using (StreamWriter writetext = File.AppendText(randomizedItemLotPath))
                                {
                                    writetext.WriteLine("Old Shop Item");
                                    writetext.WriteLine(shopLineupParams.Rows[i].Cells[0].Value);
                                    writetext.WriteLine("New Shop Item");
                                    writetext.WriteLine(shopConsumableList[randomNumber]);
                                }

                            shopLineupParams.Rows[i].Cells[0].Value = Int32.Parse(shopConsumableList[randomNumber]);

                            shopConsumableList.RemoveAt(randomNumber);
                        }
                    }
                }
            }

            //for(int i = 0; i < )

            //7000000 8 7 0 0
            //5000000 9 8 0 0
            //22000000 7 9 0 0
            //14000000 7 9 5 0
            //6000000 7 9 5 0

            //2000
            //2001
            //2002
            //2010
            //2011

            //79
            //80
            //81
            //82
            if (startingWeaponsOnlyBool)
            {
                using (StreamWriter writetext = File.AppendText(randomizedItemLotPath))
                {
                    writetext.WriteLine("Starting Required Stat Changing...");
                }

                byte nine = 9;
                byte eight = 8;
                byte seven = 7;
                byte five = 5;
                byte zero = 0;

                List<string> new2000StringList = new List<string>();
                List<string> new2001StringList = new List<string>();
                List<string> new2002StringList = new List<string>();
                List<string> new2010StringList = new List<string>();
                List<string> new2011StringList = new List<string>();

                int new2000Int = int.Parse(new2000);
                int new2001Int = int.Parse(new2001);
                int new2002Int = int.Parse(new2002);
                int new2010Int = 0;
                int new2011Int = 0;

                if (!keepGuns)
                {
                    new2010Int = int.Parse(new2010);
                    new2011Int = int.Parse(new2011);
                }

                for (int i = 0; i < 10; i++)
                {
                    new2000Int += 100;
                    new2000StringList.Add(new2000Int.ToString());

                    new2001Int += 100;
                    new2001StringList.Add(new2001Int.ToString());

                    new2002Int += 100;
                    new2002StringList.Add(new2002Int.ToString());

                    if (!keepGuns)
                    {
                        new2010Int += 100;
                        new2010StringList.Add(new2010Int.ToString());

                        new2011Int += 100;
                        new2011StringList.Add(new2011Int.ToString());
                    }
                }

                for (int i = 0; i < weaponParams.Rows.Count; i++)
                {
                    if (weaponParams.Rows[i].ID.ToString() == new2000)
                    {
                        if (logging)
                            using (StreamWriter writetext = File.AppendText(randomizedItemLotPath))
                            {
                                writetext.WriteLine("New starting item");
                                writetext.WriteLine(weaponParams.Rows[i].ID);
                            }

                        //7000000 8 7 0 0
                        weaponParams.Rows[i].Cells[80].Value = eight;
                        weaponParams.Rows[i].Cells[81].Value = seven;
                        weaponParams.Rows[i].Cells[82].Value = zero;
                        weaponParams.Rows[i].Cells[83].Value = zero;
                    }

                    if (weaponParams.Rows[i].ID.ToString() == new2001)
                    {
                        if (logging)
                            using (StreamWriter writetext = File.AppendText(randomizedItemLotPath))
                            {
                                writetext.WriteLine("New starting item");
                                writetext.WriteLine(weaponParams.Rows[i].ID);
                            }

                        //5000000 9 8 0 0
                        weaponParams.Rows[i].Cells[80].Value = nine;
                        weaponParams.Rows[i].Cells[81].Value = eight;
                        weaponParams.Rows[i].Cells[82].Value = zero;
                        weaponParams.Rows[i].Cells[83].Value = zero;
                    }

                    if (weaponParams.Rows[i].ID.ToString() == new2002)
                    {
                        if (logging)
                            using (StreamWriter writetext = File.AppendText(randomizedItemLotPath))
                            {
                                writetext.WriteLine("New starting item");
                                writetext.WriteLine(weaponParams.Rows[i].ID);
                            }

                        //22000000 7 9 0 0
                        weaponParams.Rows[i].Cells[80].Value = seven;
                        weaponParams.Rows[i].Cells[81].Value = nine;
                        weaponParams.Rows[i].Cells[82].Value = zero;
                        weaponParams.Rows[i].Cells[83].Value = zero;
                    }

                    if (!keepGuns)
                    {
                        if (weaponParams.Rows[i].ID.ToString() == new2010)
                        {
                            if (logging)
                                using (StreamWriter writetext = File.AppendText(randomizedItemLotPath))
                                {
                                    writetext.WriteLine("New starting item");
                                    writetext.WriteLine(weaponParams.Rows[i].ID);
                                }

                            //14000000 7 9 5 0
                            weaponParams.Rows[i].Cells[80].Value = seven;
                            weaponParams.Rows[i].Cells[81].Value = nine;
                            weaponParams.Rows[i].Cells[82].Value = five;
                            weaponParams.Rows[i].Cells[83].Value = zero;
                        }

                        if (weaponParams.Rows[i].ID.ToString() == new2011)
                        {
                            if (logging)
                                using (StreamWriter writetext = File.AppendText(randomizedItemLotPath))
                                {
                                    writetext.WriteLine("New starting item");
                                    writetext.WriteLine(weaponParams.Rows[i].ID);
                                }

                            //6000000 7 9 5 0
                            weaponParams.Rows[i].Cells[80].Value = seven;
                            weaponParams.Rows[i].Cells[81].Value = nine;
                            weaponParams.Rows[i].Cells[82].Value = five;
                            weaponParams.Rows[i].Cells[83].Value = zero;
                        }
                    }
                }

                //scaling the rest of the items (+ items)

                //1
                for (int i = 0; i < new2000StringList.Count; i++)
                {
                    for (int j = 0; j < weaponParams.Rows.Count; j++)
                    {
                        if (weaponParams.Rows[j].ID.ToString() == new2000StringList[i])
                        {
                            if (logging)
                                using (StreamWriter writetext = File.AppendText(randomizedItemLotPath))
                                {
                                    writetext.WriteLine("New starting item");
                                    writetext.WriteLine(weaponParams.Rows[j].ID);
                                }

                            //7000000 8 7 0 0
                            weaponParams.Rows[j].Cells[80].Value = eight;
                            weaponParams.Rows[j].Cells[81].Value = seven;
                            weaponParams.Rows[j].Cells[82].Value = zero;
                            weaponParams.Rows[j].Cells[83].Value = zero;
                        }
                    }
                }

                //2
                for (int i = 0; i < new2001StringList.Count; i++)
                {
                    for (int j = 0; j < weaponParams.Rows.Count; j++)
                    {
                        if (weaponParams.Rows[j].ID.ToString() == new2001StringList[i])
                        {
                            if (logging)
                                using (StreamWriter writetext = File.AppendText(randomizedItemLotPath))
                                {
                                    writetext.WriteLine("New starting item");
                                    writetext.WriteLine(weaponParams.Rows[j].ID);
                                }

                            //5000000 9 8 0 0
                            weaponParams.Rows[j].Cells[80].Value = nine;
                            weaponParams.Rows[j].Cells[81].Value = eight;
                            weaponParams.Rows[j].Cells[82].Value = zero;
                            weaponParams.Rows[j].Cells[83].Value = zero;
                        }
                    }
                }

                //3
                for (int i = 0; i < new2002StringList.Count; i++)
                {
                    for (int j = 0; j < weaponParams.Rows.Count; j++)
                    {
                        if (weaponParams.Rows[j].ID.ToString() == new2002StringList[i])
                        {
                            if (logging)
                                using (StreamWriter writetext = File.AppendText(randomizedItemLotPath))
                                {
                                    writetext.WriteLine("New starting item");
                                    writetext.WriteLine(weaponParams.Rows[j].ID);
                                }

                            //22000000 7 9 0 0
                            weaponParams.Rows[j].Cells[80].Value = seven;
                            weaponParams.Rows[j].Cells[81].Value = nine;
                            weaponParams.Rows[j].Cells[82].Value = zero;
                            weaponParams.Rows[j].Cells[83].Value = zero;
                        }
                    }
                }

                if (!keepGuns)
                {
                    //4
                    for (int i = 0; i < new2010StringList.Count; i++)
                    {
                        for (int j = 0; j < weaponParams.Rows.Count; j++)
                        {
                            if (weaponParams.Rows[j].ID.ToString() == new2010StringList[i])
                            {
                                if (logging)
                                    using (StreamWriter writetext = File.AppendText(randomizedItemLotPath))
                                    {
                                        writetext.WriteLine("New starting item");
                                        writetext.WriteLine(weaponParams.Rows[j].ID);
                                    }

                                //14000000 7 9 5 0
                                weaponParams.Rows[j].Cells[80].Value = seven;
                                weaponParams.Rows[j].Cells[81].Value = nine;
                                weaponParams.Rows[j].Cells[82].Value = five;
                                weaponParams.Rows[j].Cells[83].Value = zero;
                            }
                        }
                    }

                    //5
                    for (int i = 0; i < new2011StringList.Count; i++)
                    {
                        for (int j = 0; j < weaponParams.Rows.Count; j++)
                        {
                            if (weaponParams.Rows[j].ID.ToString() == new2011StringList[i])
                            {
                                if (logging)
                                    using (StreamWriter writetext = File.AppendText(randomizedItemLotPath))
                                    {
                                        writetext.WriteLine("New starting item");
                                        writetext.WriteLine(weaponParams.Rows[j].ID);
                                    }

                                //6000000 7 9 5 0
                                weaponParams.Rows[j].Cells[80].Value = seven;
                                weaponParams.Rows[j].Cells[81].Value = nine;
                                weaponParams.Rows[j].Cells[82].Value = five;
                                weaponParams.Rows[j].Cells[83].Value = zero;
                            }
                        }
                    }
                }
            }
            if (logging)
                using (StreamWriter writetext = File.AppendText(randomizedItemLotPath))
                {
                    writetext.WriteLine("Ending Required Stat Changing...");
                }

            foreach (BinderFile file in parambnd.Files)
            {
                string name = Path.GetFileNameWithoutExtension(file.Name);
                if (parms.ContainsKey(name))
                    file.Bytes = parms[name].Write();
            }
            parambnd.Write(paramPath);
        }

        private void GenerateNPCList(string currentMap)
        {
            var tempGuy = MSBB.Read(currentMap);

            for (int i = 0; i < tempGuy.Parts.Enemies.Count; i++)
            {
                if (tempGuy.Parts.Enemies[i].ModelName.Contains("c0000") && tempGuy.Parts.Enemies[i].ThinkParamID != 1)
                {
                    npcEnemyList.Add(tempGuy.Parts.Enemies[i]);
                }
            }
        }

        private void RandomizeNPCs(string currentMap)
        {
            var tempGuy = MSBB.Read(currentMap);

            for (int i = 0; i < tempGuy.Parts.Enemies.Count; i++)
            {
                if (tempGuy.Parts.Enemies[i].ModelName.Contains("c0000") && tempGuy.Parts.Enemies[i].ThinkParamID != 1)
                {
                    bool addHostile = false;


                    for (int k = 0; k < hostileNpcList.Count; k++)
                    {
                        if (tempGuy.Parts.Enemies[i].UnkT07.ToString().Contains(hostileNpcList[k]))
                        {
                            addHostile = true;
                        }
                    }


                    int random = universalRand.Next(0, npcList.Count);
                    string thisEnemy = "";

                    if (addHostile)
                    {
                        random = universalRand.Next(0, hostileNpcList.Count);

                        while (hostileNpcList[random].Contains(tempGuy.Parts.Enemies[i].UnkT07.ToString()))
                        {
                            random = universalRand.Next(0, hostileNpcList.Count);
                            thisentityID = hostileNpcList[random];
                        }
                    }
                    else
                    {
                        thisEnemy = npcList[random];
                    }

                    string tempNpcParam;
                    string tempThinkId;
                    string modelName;

                    int tempNpcParamInt;
                    int tempThinkIdInt;

                    string npc = tempGuy.Parts.Enemies[i].NPCParamID.ToString();
                    string think = tempGuy.Parts.Enemies[i].ThinkParamID.ToString();
                    string mo = tempGuy.Parts.Enemies[i].UnkT07.ToString();
                    string entityID = tempGuy.Parts.Enemies[i].EntityID.ToString();

                    int tempnpcint = thisEnemy.IndexOf("*");
                    tempNpcParam = thisEnemy.Substring(0, tempnpcint);
                    tempThinkId = thisEnemy.Substring(tempnpcint + 1, thisEnemy.LastIndexOf("*") - tempnpcint - 1);
                    modelName = thisEnemy.Substring(thisEnemy.LastIndexOf("*") + 1, 4);

                    tempNpcParamInt = int.Parse(tempNpcParam);
                    tempThinkIdInt = int.Parse(tempThinkId);

                    if (tempGuy.Parts.Enemies[i].NPCParamID != 6380)
                    {
                        if (logging)
                            using (StreamWriter writetext = File.AppendText(randomizedNPCPath))
                            {
                                writetext.WriteLine(currentMap);
                                writetext.WriteLine(npcList.Count);
                                writetext.WriteLine("Old NPC");
                                writetext.WriteLine(npc + " npcParam");
                                writetext.WriteLine(think + " thinkID");
                                writetext.WriteLine(mo + " UNKT07");

                                writetext.WriteLine("New NPC");
                                writetext.WriteLine(tempNpcParam + " npcParam");
                                writetext.WriteLine(tempThinkId + " thinkID");
                                writetext.WriteLine(modelName + " UNKT07");

                                writetext.WriteLine("" + Environment.NewLine + Environment.NewLine);
                            }

                        tempGuy.Parts.Enemies[i].NPCParamID = tempNpcParamInt;
                        tempGuy.Parts.Enemies[i].ThinkParamID = tempThinkIdInt;
                        tempGuy.Parts.Enemies[i].UnkT07 = int.Parse(modelName);
                    }
                    else if (tempGuy.Parts.Enemies[i].NPCParamID == 6380)
                    {
                        int randoHostile = universalRand.Next(0, hostileNpcList.Count);

                        thisEnemy = hostileNpcList[randoHostile];

                        npc = tempGuy.Parts.Enemies[i].NPCParamID.ToString();
                        think = tempGuy.Parts.Enemies[i].ThinkParamID.ToString();
                        mo = tempGuy.Parts.Enemies[i].UnkT07.ToString();
                        entityID = tempGuy.Parts.Enemies[i].EntityID.ToString();

                        tempnpcint = thisEnemy.IndexOf("*");
                        tempNpcParam = thisEnemy.Substring(0, tempnpcint);
                        tempThinkId = thisEnemy.Substring(tempnpcint + 1, thisEnemy.LastIndexOf("*") - tempnpcint - 1);
                        modelName = thisEnemy.Substring(thisEnemy.LastIndexOf("*") + 1, 4);

                        tempNpcParamInt = int.Parse(tempNpcParam);
                        tempThinkIdInt = int.Parse(tempThinkId);

                        if (logging)
                            using (StreamWriter writetext = File.AppendText(randomizedNPCPath))
                            {
                                writetext.WriteLine(currentMap);
                                writetext.WriteLine(npcList.Count);
                                writetext.WriteLine("Old NPC");
                                writetext.WriteLine(npc + " npcParam");
                                writetext.WriteLine(think + " thinkID");
                                writetext.WriteLine(mo + "UNKT07");

                                writetext.WriteLine("New NPC");
                                writetext.WriteLine(tempNpcParam + " npcParam");
                                writetext.WriteLine(tempThinkId + " thinkID");
                                writetext.WriteLine(modelName + " UNKT07");

                                writetext.WriteLine("" + Environment.NewLine + Environment.NewLine);
                            }

                        tempGuy.Parts.Enemies[i].NPCParamID = tempNpcParamInt;
                        tempGuy.Parts.Enemies[i].ThinkParamID = tempThinkIdInt;
                        tempGuy.Parts.Enemies[i].UnkT07 = int.Parse(modelName);
                    }

                    npcList.RemoveAt(random);
                }
            }

            tempGuy.Write(currentMap);
        }

        private void InsertBossesVoid(string currentMap, List<string> nonoList)
        {
            for (int i = 0; i < insertBossesString.Count; i++)
            {
                if (insertBossesString[i].Contains("c5400"))
                {
                    insertBossesString.RemoveAt(i);
                    i = insertBossesString.Count + 3;
                }
            }

            if (randomize)
            {
                var tempGUY = MSBB.Read(currentMap);

                bool changeData;

                for (int i = 0; i < tempGUY.Parts.Enemies.Count; i++)
                {
                    changeData = true;

                    for (int j = 0; j < nonoList.Count; j++)
                    {
                        if (tempGUY.Parts.Enemies[i].Name.Contains(nonoList[j]))
                        {
                            changeData = false;
                        }
                    }


                    if (changeData && insertBossesString.Count > 0)
                    {

                        int random = universalRand.Next(0, insertBossesString.Count);
                        int randomChalice = universalRand.Next(0, chaliceBossString.Count);
                        string thisEnemy;
                        if (currentMap.Contains("m29"))
                        {
                            thisEnemy = chaliceBossString[randomChalice];
                        }
                        else
                        {
                            thisEnemy = insertBossesString[random];
                        }


                        string tempNpcParam;
                        string tempThinkId;
                        string modelName;

                        int tempNpcParamInt;
                        int tempThinkIdInt;

                        Console.WriteLine(enemyData[0]);

                        int tempnpcint = thisEnemy.IndexOf("*");
                        tempNpcParam = thisEnemy.Substring(0, tempnpcint);
                        tempThinkId = thisEnemy.Substring(tempnpcint + 1, thisEnemy.LastIndexOf("*") - tempnpcint - 1);
                        modelName = thisEnemy.Substring(thisEnemy.LastIndexOf("*") + 1, 5);

                        tempNpcParamInt = int.Parse(tempNpcParam);
                        tempThinkIdInt = int.Parse(tempThinkId);
                        float assass = universalRand.Next(0, 101);
                        if (assass <= bossPercentage)
                        {
                            if (logging)
                                using (StreamWriter writetext = File.AppendText(insertedBossPath))
                                {
                                    writetext.WriteLine(currentMap);
                                    writetext.WriteLine("Inserted Boss");
                                    writetext.WriteLine(tempNpcParam + " npcParam");
                                    writetext.WriteLine(tempThinkId + " thinkID");
                                    writetext.WriteLine(modelName + " model");

                                    writetext.WriteLine("" + Environment.NewLine + Environment.NewLine);
                                }

                            tempGUY.Parts.Enemies[i].NPCParamID = tempNpcParamInt;
                            if (tempNpcParamInt == 551000)
                            {
                                tempThinkIdInt = 551111;
                            }
                            else
                            {
                                tempGUY.Parts.Enemies[i].ThinkParamID = tempThinkIdInt;
                            }
                            tempGUY.Parts.Enemies[i].ModelName = modelName;
                        }
                    }
                }
                tempGUY.Write(currentMap);
            }
        }

        private void GenerateBossList(string currentMap, List<string> noNoList)
        {


            var tempGUY = MSBB.Read(currentMap);

            List<MSBB.Part.Enemy> tempList = new List<MSBB.Part.Enemy>();


            bool addEnemy;

            if (!oopsAll)
            {
                for (int i = 0; i < tempGUY.Parts.Enemies.Count; i++)
                {
                    addEnemy = false;

                    if (tempGUY.Parts.Enemies[i].Name.Contains("c4540_0000"))
                    {
                        OoKEnemy = tempGUY.Parts.Enemies[i];
                    }

                    if (currentMap.Contains("m29"))
                    {
                        for (int k = 0; k < chaliceBossParams.Count; k++)
                        {
                            if (tempGUY.Parts.Enemies[i].NPCParamID.ToString() == chaliceBossParams[k])
                            {
                                newChaliceBossMSB.Add(tempGUY.Parts.Enemies[i]);
                            }
                        }
                    }

                    for (int j = 0; j < bossList.Count; j++)
                    {
                        if (tempGUY.Parts.Enemies[i].Name.Contains(bossList[j]))
                        {
                            if (tempGUY.Parts.Enemies[i].NPCParamID == 250060)
                            {
                                addEnemy = false;
                            }
                            else if (tempGUY.Parts.Enemies[i].NPCParamID == 250070)
                            {
                                addEnemy = false;
                            }
                            else if (tempGUY.Parts.Enemies[i].NPCParamID == 250090)
                            {
                                addEnemy = false;
                            }
                            else if (tempGUY.Parts.Enemies[i].NPCParamID == 212600)
                            {
                                addEnemy = false;
                            }
                            else if (tempGUY.Parts.Enemies[i].NPCParamID == 212610)
                            {
                                addEnemy = false;
                            }
                            else if (tempGUY.Parts.Enemies[i].NPCParamID == 212620)
                            {
                                addEnemy = false;
                            }
                            else if (tempGUY.Parts.Enemies[i].EntityID == -1)
                            {
                                addEnemy = false;
                            }
                            else if (tempList.Contains(tempGUY.Parts.Enemies[i]))
                            {
                                addEnemy = false;
                            }
                            else if (tempGUY.Parts.Enemies[i].NPCParamID == 0 || tempGUY.Parts.Enemies[i].NPCParamID == -1)
                            {
                                addEnemy = false;
                            }
                            else if (tempGUY.Parts.Enemies[i].ThinkParamID == -1)
                            {
                                addEnemy = false;
                            }
                            else if (tempGUY.Parts.Enemies[i].Name.Contains("c5070") && !lesserBossesBool)
                            {
                                addEnemy = false;
                            }
                            else if (tempGUY.Parts.Enemies[i].Name.Contains("c3060") && tempGUY.Parts.Enemies[i].NPCParamID != 210306016)
                            {
                                addEnemy = false;
                            }
                            else
                            {
                                addEnemy = true;

                                if (currentMap.Contains("m29"))
                                {
                                    for (int k = 0; k < chaliceBossParams.Count; k++)
                                    {
                                        if (tempGUY.Parts.Enemies[i].NPCParamID.ToString() == chaliceBossParams[k])
                                        {
                                            chaliceBossMSB.Add(tempGUY.Parts.Enemies[i]);
                                            insertBossesEnemy.Add(tempGUY.Parts.Enemies[i]);
                                        }
                                    }
                                }

                                if (!currentMap.Contains("m29"))
                                {
                                    insertBossesEnemy.Add(tempGUY.Parts.Enemies[i]);
                                }

                                if (!currentMap.Contains("m26"))
                                {
                                    if (tempGUY.Parts.Enemies[i].Name.Contains("c0000_0005"))
                                    {
                                        addEnemy = false;
                                    }
                                }
                            }

                            if (currentMap.Contains("34"))
                            {
                                if (tempGUY.Parts.Enemies[i].NPCParamID == 210030)
                                {
                                    addEnemy = false;
                                }
                            }

                            if (currentMap.Contains("m28"))
                            {
                                if (tempGUY.Parts.Enemies[i].Name.Contains("c2100"))
                                {
                                    addEnemy = false;
                                }
                            }


                        }
                    }

                    if (addEnemy)
                    {
                        if (randomize)
                        {
                            if (!tempList.Contains(tempGUY.Parts.Enemies[i]))
                            {
                                if (!tempGUY.Parts.Enemies[i].Name.Contains("c2500_0000") && !tempGUY.Parts.Enemies[i].Name.Contains("c5071_0000")
                                    && !tempGUY.Parts.Enemies[i].Name.Contains("c4030_0004") && !tempGUY.Parts.Enemies[i].Name.Contains("c2100_0000")
                                    && !tempGUY.Parts.Enemies[i].Name.Contains("c2100_0001") && !tempGUY.Parts.Enemies[i].Name.Contains("c4540_0000")
                                    && !tempGUY.Parts.Enemies[i].Name.Contains("c4030_0001") && !tempGUY.Parts.Enemies[i].Name.Contains("c4030_0002")
                                    && !tempGUY.Parts.Enemies[i].Name.Contains("c4030_0003") && !tempGUY.Parts.Enemies[i].Name.Contains("c2120_0001")
                                    && !tempGUY.Parts.Enemies[i].Name.Contains("c2120_0000") && !tempGUY.Parts.Enemies[i].Name.Contains("c2120_0002")
                                    && !tempGUY.Parts.Enemies[i].Name.Contains("c2570") && !tempGUY.Parts.Enemies[i].Name.Contains("c2571")
                                    && !tempGUY.Parts.Enemies[i].Name.Contains("c4030_0000") && !lesserBossesBool)
                                {
                                    bool add = true;

                                    for (int n = 0; n < tempList.Count; n++)
                                    {
                                        if (tempGUY.Parts.Enemies[i].NPCParamID == tempList[n].NPCParamID)
                                        {
                                            add = false;
                                        }
                                    }
                                    if (add)
                                    {
                                        bool addThisBoss = true;
                                        if (currentMap.Contains("m29"))
                                        {
                                            if (addedBossesList.Count > 0 && addedBossesList != null)
                                            {
                                                for (int n = 0; n < addedBossesList.Count; n++)
                                                {
                                                    if (tempGUY.Parts.Enemies[i].ModelName == addedBossesList[n].ModelName)
                                                    {
                                                        addThisBoss = false;
                                                    }
                                                }
                                                if (addThisBoss)
                                                {
                                                    tempList.Add(tempGUY.Parts.Enemies[i]);
                                                    addedBossesList.Add(tempGUY.Parts.Enemies[i]);
                                                }
                                            }
                                        }
                                        else
                                        {
                                            tempList.Add(tempGUY.Parts.Enemies[i]);
                                            addedBossesList.Add(tempGUY.Parts.Enemies[i]);
                                        }
                                    }
                                }
                                else if (!tempGUY.Parts.Enemies[i].Name.Contains("c2500_0000") && !tempGUY.Parts.Enemies[i].Name.Contains("c5071_0000")
                                    && !tempGUY.Parts.Enemies[i].Name.Contains("c4540_0000") && !tempGUY.Parts.Enemies[i].Name.Contains("c2571")
                                    && lesserBossesBool)
                                {
                                    bool add = true;

                                    for (int n = 0; n < tempList.Count; n++)
                                    {
                                        if (tempGUY.Parts.Enemies[i].NPCParamID == tempList[n].NPCParamID)
                                        {
                                            add = false;
                                        }
                                    }
                                    if (add)
                                    {
                                        bool addThisBoss = true;
                                        if (currentMap.Contains("m29"))
                                        {
                                            if (addedBossesList.Count > 0 && addedBossesList != null)
                                            {
                                                for (int n = 0; n < addedBossesList.Count; n++)
                                                {
                                                    if (tempGUY.Parts.Enemies[i].ModelName == addedBossesList[n].ModelName)
                                                    {
                                                        addThisBoss = false;
                                                    }
                                                }
                                                if (addThisBoss)
                                                {
                                                    tempList.Add(tempGUY.Parts.Enemies[i]);
                                                    addedBossesList.Add(tempGUY.Parts.Enemies[i]);
                                                }
                                            }
                                        }
                                        else
                                        {
                                            tempList.Add(tempGUY.Parts.Enemies[i]);
                                            addedBossesList.Add(tempGUY.Parts.Enemies[i]);
                                        }
                                    }
                                }
                                totalBossesToReplace++;
                            }
                        }
                    }
                }

                for (int i = 0; i < tempList.Count; i++)
                {
                    string tempString = tempList[i].NPCParamID.ToString() + "*" + tempList[i].ThinkParamID.ToString() + "*" + tempList[i].ModelName;
                    if (tempList[i].ThinkParamID.ToString() != "1")
                    {
                        enemyData.Add(tempList[i].NPCParamID.ToString() + "*" + tempList[i].ThinkParamID.ToString() + "*" + tempList[i].ModelName);

                    }
                }

                if (currentMap.Contains("m36"))
                {
                    OoKFirstPhase = OoKEnemy.NPCParamID.ToString() + "*" + OoKEnemy.ThinkParamID.ToString() + "*" + OoKEnemy.ModelName;
                }

                for (int i = 0; i < newChaliceBossMSB.Count; i++)
                {
                    string tempString = newChaliceBossMSB[i].NPCParamID.ToString() + "*" + newChaliceBossMSB[i].ThinkParamID.ToString() + "*" + newChaliceBossMSB[i].ModelName;
                    newChaliceBossString.Add(newChaliceBossMSB[i].NPCParamID.ToString() + "*" + newChaliceBossMSB[i].ThinkParamID.ToString() + "*" + newChaliceBossMSB[i].ModelName);
                    newChaliceBossString2.Add(newChaliceBossMSB[i].NPCParamID.ToString() + "*" + newChaliceBossMSB[i].ThinkParamID.ToString() + "*" + newChaliceBossMSB[i].ModelName);
                }

                for (int i = 0; i < chaliceBossMSB.Count; i++)
                {
                    string tempString = chaliceBossMSB[i].NPCParamID.ToString() + "*" + chaliceBossMSB[i].ThinkParamID.ToString() + "*" + chaliceBossMSB[i].ModelName;
                    if (chaliceBossMSB[i].ThinkParamID.ToString() != "1")
                    {
                        if (excludeBossesBool)
                        {
                            bool add = true;
                            for (int j = 0; j < oopsAllBossString.Count; j++)
                            {
                                if (chaliceBossMSB[i].ModelName.Contains(oopsAllBossString[j]))
                                {
                                    add = false;
                                }

                            }
                            if (add)
                            {
                                chaliceBossString.Add(chaliceBossMSB[i].NPCParamID.ToString() + "*" + chaliceBossMSB[i].ThinkParamID.ToString() + "*" + chaliceBossMSB[i].ModelName);
                            }
                        }
                        else
                        {
                            chaliceBossString.Add(chaliceBossMSB[i].NPCParamID.ToString() + "*" + chaliceBossMSB[i].ThinkParamID.ToString() + "*" + chaliceBossMSB[i].ModelName);
                        }
                    }
                }

                for (int i = 0; i < insertBossesEnemy.Count; i++)
                {
                    string tempString = insertBossesEnemy[i].NPCParamID.ToString() + "*" + insertBossesEnemy[i].ThinkParamID.ToString() + "*" + insertBossesEnemy[i].ModelName;
                    if (insertBossesEnemy[i].ThinkParamID.ToString() != "1")
                    {
                        insertBossesString.Add(insertBossesEnemy[i].NPCParamID.ToString() + "*" + insertBossesEnemy[i].ThinkParamID.ToString() + "*" + insertBossesEnemy[i].ModelName);
                    }
                }
            }
        }

        private void AddOrphanPhaseOne(string currentMap, List<string> nonoList)
        {
            var tempGUY = MSBB.Read(currentMap);

            for (int i = 0; i < tempGUY.Parts.Enemies.Count; i++)
            {


                if (currentMap.Contains("m24_01"))
                {
                    if (tempGUY.Parts.Enemies[i].Name.Contains("c2710_0000"))
                    {
                        thisnpc = tempGUY.Parts.Enemies[i].NPCParamID.ToString();
                        thisthink = tempGUY.Parts.Enemies[i].ThinkParamID.ToString();
                        thismo = tempGUY.Parts.Enemies[i].ModelName;
                        thisentityID = tempGUY.Parts.Enemies[i].EntityID.ToString();

                        string thisEnemy = OoKFirstPhase;

                        string tempNpcParam;
                        string tempThinkId;
                        string modelName;

                        int tempNpcParamInt;
                        int tempThinkIdInt;

                        int tempnpcint = thisEnemy.IndexOf("*");
                        tempNpcParam = thisEnemy.Substring(0, tempnpcint);
                        tempThinkId = thisEnemy.Substring(tempnpcint + 1, thisEnemy.LastIndexOf("*") - tempnpcint - 1);
                        modelName = thisEnemy.Substring(thisEnemy.LastIndexOf("*") + 1, 5);

                        tempNpcParamInt = int.Parse(tempNpcParam);
                        tempThinkIdInt = int.Parse(tempThinkId);

                        tempGUY.Parts.Enemies[i].NPCParamID = tempNpcParamInt;
                        tempGUY.Parts.Enemies[i].ThinkParamID = tempThinkIdInt;
                        tempGUY.Parts.Enemies[i].ModelName = modelName;

                        if (logging)
                            using (StreamWriter writetext = File.AppendText(bossLogFilePath))
                            {
                                writetext.WriteLine(bossCount);
                                writetext.WriteLine(currentMap);
                                writetext.WriteLine(i + " Number in map list");
                                writetext.WriteLine("Old Boss");
                                writetext.WriteLine(thisnpc + " npcParam");
                                writetext.WriteLine(thisthink + " thinkID");
                                writetext.WriteLine(thismo + " model");
                                writetext.WriteLine(thisentityID);

                                writetext.WriteLine("New Boss");
                                writetext.WriteLine(tempNpcParam + " npcParam");
                                writetext.WriteLine(tempThinkId + " thinkID");
                                writetext.WriteLine(modelName + " new model");
                                writetext.WriteLine("This is first phase Orphan.");
                            }
                    }
                }
                else if (currentMap.Contains("m24_02"))
                {
                    if (tempGUY.Parts.Enemies[i].Name.Contains("c2500_0000"))
                    {
                        thisnpc = tempGUY.Parts.Enemies[i].NPCParamID.ToString();
                        thisthink = tempGUY.Parts.Enemies[i].ThinkParamID.ToString();
                        thismo = tempGUY.Parts.Enemies[i].ModelName;
                        thisentityID = tempGUY.Parts.Enemies[i].EntityID.ToString();

                        string thisEnemy = OoKFirstPhase;

                        string tempNpcParam;
                        string tempThinkId;
                        string modelName;

                        int tempNpcParamInt;
                        int tempThinkIdInt;

                        int tempnpcint = thisEnemy.IndexOf("*");
                        tempNpcParam = thisEnemy.Substring(0, tempnpcint);
                        tempThinkId = thisEnemy.Substring(tempnpcint + 1, thisEnemy.LastIndexOf("*") - tempnpcint - 1);
                        modelName = thisEnemy.Substring(thisEnemy.LastIndexOf("*") + 1, 5);

                        tempNpcParamInt = int.Parse(tempNpcParam);
                        tempThinkIdInt = int.Parse(tempThinkId);

                        tempGUY.Parts.Enemies[i].NPCParamID = tempNpcParamInt;
                        tempGUY.Parts.Enemies[i].ThinkParamID = tempThinkIdInt;
                        tempGUY.Parts.Enemies[i].ModelName = modelName;

                        if (logging)
                            using (StreamWriter writetext = File.AppendText(bossLogFilePath))
                            {
                                writetext.WriteLine(bossCount);
                                writetext.WriteLine(currentMap);
                                writetext.WriteLine(i + " Number in map list");
                                writetext.WriteLine("Old Boss");
                                writetext.WriteLine(thisnpc + " npcParam");
                                writetext.WriteLine(thisthink + " thinkID");
                                writetext.WriteLine(thismo + " model");
                                writetext.WriteLine(thisentityID);

                                writetext.WriteLine("New Boss");
                                writetext.WriteLine(tempNpcParam + " npcParam");
                                writetext.WriteLine(tempThinkId + " thinkID");
                                writetext.WriteLine(modelName + " new model");
                                writetext.WriteLine("This is first phase Orphan.");
                            }
                    }
                }
                else if (currentMap.Contains("m34_00"))
                {
                    if (tempGUY.Parts.Enemies[i].Name.Contains("c4510_0000"))
                    {
                        thisnpc = tempGUY.Parts.Enemies[i].NPCParamID.ToString();
                        thisthink = tempGUY.Parts.Enemies[i].ThinkParamID.ToString();
                        thismo = tempGUY.Parts.Enemies[i].ModelName;
                        thisentityID = tempGUY.Parts.Enemies[i].EntityID.ToString();

                        string thisEnemy = OoKFirstPhase;

                        string tempNpcParam;
                        string tempThinkId;
                        string modelName;

                        int tempNpcParamInt;
                        int tempThinkIdInt;

                        int tempnpcint = thisEnemy.IndexOf("*");
                        tempNpcParam = thisEnemy.Substring(0, tempnpcint);
                        tempThinkId = thisEnemy.Substring(tempnpcint + 1, thisEnemy.LastIndexOf("*") - tempnpcint - 1);
                        modelName = thisEnemy.Substring(thisEnemy.LastIndexOf("*") + 1, 5);

                        tempNpcParamInt = int.Parse(tempNpcParam);
                        tempThinkIdInt = int.Parse(tempThinkId);

                        tempGUY.Parts.Enemies[i].NPCParamID = tempNpcParamInt;
                        tempGUY.Parts.Enemies[i].ThinkParamID = tempThinkIdInt;
                        tempGUY.Parts.Enemies[i].ModelName = modelName;

                        if (logging)
                            using (StreamWriter writetext = File.AppendText(bossLogFilePath))
                            {
                                writetext.WriteLine(bossCount);
                                writetext.WriteLine(currentMap);
                                writetext.WriteLine(i + " Number in map list");
                                writetext.WriteLine("Old Boss");
                                writetext.WriteLine(thisnpc + " npcParam");
                                writetext.WriteLine(thisthink + " thinkID");
                                writetext.WriteLine(thismo + " model");
                                writetext.WriteLine(thisentityID);

                                writetext.WriteLine("New Boss");
                                writetext.WriteLine(tempNpcParam + " npcParam");
                                writetext.WriteLine(tempThinkId + " thinkID");
                                writetext.WriteLine(modelName + " new model");
                                writetext.WriteLine("This is first phase Orphan.");
                            }
                    }
                }
            }

            tempGUY.Write(currentMap);
            tempGUY.Write(currentMap);
        }
        private void RandomizeBosses(string currentMap, List<string> nonoList)
        {
            int removalNumber = 0;

            if (randomize)
            {
                var tempGUY = MSBB.Read(currentMap);

                bool changeData;

                int addCounter = 0;


                if (!oopsAllBosses)
                {
                    for (int i = 0; i < tempGUY.Parts.Enemies.Count; i++)
                    {

                        if (combinedBossList.Count <= 0)
                        {
                            combinedBossList = new List<string>();

                            for (int j = 0; j < combinedBossList2.Count; j++)
                            {
                                combinedBossList.Add(combinedBossList2[j]);
                            }
                        }

                        changeData = false;

                        for (int j = 0; j < bossList.Count; j++)
                        {
                            if (tempGUY.Parts.Enemies[i].Name.Contains(bossList[j]))
                            {
                                if (tempGUY.Parts.Enemies[i].NPCParamID == 250060)
                                {
                                    changeData = false;
                                }
                                else if (tempGUY.Parts.Enemies[i].NPCParamID == 250070)
                                {
                                    changeData = false;
                                }
                                else if (tempGUY.Parts.Enemies[i].NPCParamID == 250090)
                                {
                                    changeData = false;
                                }
                                else if (tempGUY.Parts.Enemies[i].NPCParamID == 212600)
                                {
                                    changeData = false;
                                }
                                else if (tempGUY.Parts.Enemies[i].NPCParamID == 212610)
                                {
                                    changeData = false;
                                }
                                else if (tempGUY.Parts.Enemies[i].NPCParamID == 212620)
                                {
                                    changeData = false;
                                }
                                else if (tempGUY.Parts.Enemies[i].EntityID == -1)
                                {
                                    changeData = false;
                                }
                                else if (tempGUY.Parts.Enemies[i].NPCParamID == 0 || tempGUY.Parts.Enemies[i].NPCParamID == -1)
                                {
                                    changeData = false;
                                }
                                else if (tempGUY.Parts.Enemies[i].Name.Contains("c3060") && !currentMap.Contains("m29"))
                                {
                                    changeData = false;
                                }
                                else if (!currentMap.Contains("m36") && tempGUY.Parts.Enemies[i].ThinkParamID == 454000)
                                {
                                    changeData = false;
                                }
                                else if (currentMap.Contains("m22") && tempGUY.Parts.Enemies[i].Name.Contains("c2100_0001"))
                                {
                                    changeData = false;
                                }
                                else if (currentMap.Contains("m27") && tempGUY.Parts.Enemies[i].Name.Contains("c2120_0001"))
                                {
                                    changeData = false;
                                }
                                else if (currentMap.Contains("m27") && tempGUY.Parts.Enemies[i].Name.Contains("c2120_0002"))
                                {
                                    changeData = false;
                                }
                                else if (currentMap.Contains("m35") && tempGUY.Parts.Enemies[i].Name.Contains("c4030_0001"))
                                {
                                    changeData = false;
                                }
                                else if (currentMap.Contains("m35") && tempGUY.Parts.Enemies[i].Name.Contains("c4030_0002"))
                                {
                                    changeData = false;
                                }
                                else if (currentMap.Contains("m35") && tempGUY.Parts.Enemies[i].Name.Contains("c4030_0003"))
                                {
                                    changeData = false;
                                }
                                else if (!currentMap.Contains("m26") && tempGUY.Parts.Enemies[i].Name.Contains("c0000_0005"))
                                {
                                    changeData = false;
                                    List<long> tempLong = new List<long>();
                                    tempLong.Add(tempGUY.Parts.Enemies[i].NPCParamID);
                                }
                                else if (tempGUY.Parts.Enemies[i].Name.Contains("c3060") && !currentMap.Contains("m29"))
                                {
                                    changeData = false;
                                }
                                else if (tempGUY.Parts.Enemies[i].Name.Contains("c4030_0004") && currentMap.Contains("m35"))
                                {
                                    changeData = false;
                                }
                                else
                                {
                                    changeData = true;
                                    thisnpc = tempGUY.Parts.Enemies[i].NPCParamID.ToString();
                                    thisthink = tempGUY.Parts.Enemies[i].ThinkParamID.ToString();
                                    thismo = tempGUY.Parts.Enemies[i].ModelName;
                                    thisentityID = tempGUY.Parts.Enemies[i].EntityID.ToString();
                                }

                                if (currentMap.Contains("34"))
                                {
                                    if (tempGUY.Parts.Enemies[i].NPCParamID == 210030)
                                    {
                                        changeData = false;
                                    }
                                }

                                if (currentMap.Contains("m28"))
                                {
                                    if (tempGUY.Parts.Enemies[i].Name.Contains("c2100"))
                                    {
                                        changeData = false;
                                    }
                                }
                            }
                        }

                        if (currentMap.Contains("m29"))
                        {
                            changeData = false;

                            for (int k = 0; k < chaliceBossParams.Count; k++)
                            {
                                if (tempGUY.Parts.Enemies[i].NPCParamID.ToString() == chaliceBossParams[k])
                                {

                                    int random = universalRand.Next(0, newChaliceBossString.Count);
                                    string thisEnemy;

                                    thisEnemy = newChaliceBossString[random];

                                    removalNumber = random;

                                    string tempNpcParam;
                                    string tempThinkId;
                                    string modelName;

                                    int tempNpcParamInt;
                                    int tempThinkIdInt;

                                    int tempnpcint = thisEnemy.IndexOf("*");
                                    tempNpcParam = thisEnemy.Substring(0, tempnpcint);
                                    tempThinkId = thisEnemy.Substring(tempnpcint + 1, thisEnemy.LastIndexOf("*") - tempnpcint - 1);
                                    modelName = thisEnemy.Substring(thisEnemy.LastIndexOf("*") + 1, 5);

                                    tempNpcParamInt = int.Parse(tempNpcParam);
                                    tempThinkIdInt = int.Parse(tempThinkId);

                                    tempGUY.Parts.Enemies[i].NPCParamID = tempNpcParamInt;
                                    tempGUY.Parts.Enemies[i].ThinkParamID = tempThinkIdInt;
                                    tempGUY.Parts.Enemies[i].ModelName = modelName;

                                    while (tempGUY.Parts.Enemies[i].Name.Contains(modelName))
                                    {
                                        random = universalRand.Next(0, newChaliceBossString.Count);
                                        thisEnemy = newChaliceBossString[random];
                                        removalNumber = random;

                                        tempnpcint = thisEnemy.IndexOf("*");
                                        tempNpcParam = thisEnemy.Substring(0, tempnpcint);
                                        tempThinkId = thisEnemy.Substring(tempnpcint + 1, thisEnemy.LastIndexOf("*") - tempnpcint - 1);
                                        modelName = thisEnemy.Substring(thisEnemy.LastIndexOf("*") + 1, 5);
                                        tempNpcParamInt = int.Parse(tempNpcParam);
                                        tempThinkIdInt = int.Parse(tempThinkId);

                                        addCounter++;
                                        if (addCounter >= newChaliceBossString.Count)
                                        {
                                            //enemyDataRandomized.Add(secondaryBosses[random]);
                                            addCounter = 0;
                                        }
                                    }

                                    string npc = tempGUY.Parts.Enemies[i].NPCParamID.ToString();
                                    string think = tempGUY.Parts.Enemies[i].ThinkParamID.ToString();
                                    string mo = tempGUY.Parts.Enemies[i].ModelName;
                                    string entityID = tempGUY.Parts.Enemies[i].EntityID.ToString();

                                    thisnpc = npc;
                                    thisthink = think;
                                    thismo = mo;
                                    thisentityID = entityID;

                                    tempGUY.Parts.Enemies[i].NPCParamID = tempNpcParamInt;
                                    tempGUY.Parts.Enemies[i].ThinkParamID = tempThinkIdInt;
                                    tempGUY.Parts.Enemies[i].ModelName = modelName;

                                    if (logging)
                                        using (StreamWriter writetext = File.AppendText(bossLogFilePath))
                                        {
                                            writetext.WriteLine(bossCount);
                                            writetext.WriteLine(currentMap);
                                            writetext.WriteLine(i + " Number in map list");
                                            writetext.WriteLine("Old Boss");
                                            writetext.WriteLine(thisnpc + " npcParam");
                                            writetext.WriteLine(thisthink + " thinkID");
                                            writetext.WriteLine(thismo + " model");
                                            writetext.WriteLine(thisentityID);

                                            writetext.WriteLine("New Boss");
                                            writetext.WriteLine(tempNpcParam + " npcParam");
                                            writetext.WriteLine(tempThinkId + " thinkID");
                                            writetext.WriteLine(modelName + " new model");


                                            writetext.WriteLine("Enemy Pool Count: " + newChaliceBossString.Count);

                                            for (int m = newChaliceBossString.Count - 1; m >= 0; m--)
                                            {
                                                if (newChaliceBossString[m].Contains("*" + modelName))
                                                {
                                                    newChaliceBossString.RemoveAt(m);
                                                }
                                            }

                                            writetext.WriteLine("New count after removing similar bosses " + newChaliceBossString.Count + Environment.NewLine + Environment.NewLine);

                                            if (newChaliceBossString.Count <= 0 || newChaliceBossString == null)
                                            {
                                                newChaliceBossString = new List<string>();

                                                for (int j = 0; j < newChaliceBossString2.Count; j++)
                                                {
                                                    newChaliceBossString.Add(newChaliceBossString2[j]);
                                                }
                                            }
                                        }
                                }
                            }
                        }



                        if (changeData && combinedBossList.Count > 0)
                        {
                            oldNPCParam.Add(tempGUY.Parts.Enemies[i].NPCParamID);


                            int random = universalRand.Next(0, combinedBossList.Count);
                            string thisEnemy;

                            thisEnemy = combinedBossList[random];

                            removalNumber = random;

                            string tempNpcParam;
                            string tempThinkId;
                            string modelName;

                            int tempNpcParamInt;
                            int tempThinkIdInt;

                            int tempnpcint = thisEnemy.IndexOf("*");
                            tempNpcParam = thisEnemy.Substring(0, tempnpcint);
                            tempThinkId = thisEnemy.Substring(tempnpcint + 1, thisEnemy.LastIndexOf("*") - tempnpcint - 1);
                            modelName = thisEnemy.Substring(thisEnemy.LastIndexOf("*") + 1, 5);

                            tempNpcParamInt = int.Parse(tempNpcParam);
                            tempThinkIdInt = int.Parse(tempThinkId);


                            if (currentMap.Contains("m24_00"))
                            {
                                while (modelName.Contains("4500") || modelName.Contains("4520"))
                                {
                                    random = universalRand.Next(0, combinedBossList.Count);
                                    thisEnemy = combinedBossList[random];
                                    tempnpcint = thisEnemy.IndexOf("*");
                                    tempNpcParam = thisEnemy.Substring(0, tempnpcint);
                                    tempThinkId = thisEnemy.Substring(tempnpcint + 1, thisEnemy.LastIndexOf("*") - tempnpcint - 1);
                                    modelName = thisEnemy.Substring(thisEnemy.LastIndexOf("*") + 1, 5);
                                }
                            }

                            if (currentMap.Contains("m34_00"))
                            {
                                while (modelName.Contains("3060"))
                                {
                                    random = universalRand.Next(0, combinedBossList.Count);
                                    thisEnemy = combinedBossList[random];
                                    tempnpcint = thisEnemy.IndexOf("*");
                                    tempNpcParam = thisEnemy.Substring(0, tempnpcint);
                                    tempThinkId = thisEnemy.Substring(tempnpcint + 1, thisEnemy.LastIndexOf("*") - tempnpcint - 1);
                                    modelName = thisEnemy.Substring(thisEnemy.LastIndexOf("*") + 1, 5);
                                }
                            }

                            if (currentMap.Contains("m24_02"))
                            {
                                while (modelName.Contains("5100") || modelName.Contains("8050") || modelName.Contains("4520") || modelName.Contains("4500") || modelName.Contains("3060"))
                                {
                                    random = universalRand.Next(0, combinedBossList.Count);
                                    thisEnemy = combinedBossList[random];
                                    tempnpcint = thisEnemy.IndexOf("*");
                                    tempNpcParam = thisEnemy.Substring(0, tempnpcint);
                                    tempThinkId = thisEnemy.Substring(tempnpcint + 1, thisEnemy.LastIndexOf("*") - tempnpcint - 1);
                                    modelName = thisEnemy.Substring(thisEnemy.LastIndexOf("*") + 1, 5);
                                }
                            }


                            tempGUY.Parts.Enemies[i].NPCParamID = tempNpcParamInt;
                            tempGUY.Parts.Enemies[i].ThinkParamID = tempThinkIdInt;
                            tempGUY.Parts.Enemies[i].ModelName = modelName;

                            if (!currentMap.Contains("m29"))
                            {
                                while (tempGUY.Parts.Enemies[i].Name.Contains(modelName))
                                {
                                    random = universalRand.Next(0, combinedBossList.Count);
                                    thisEnemy = combinedBossList[random];
                                    removalNumber = random;

                                    tempnpcint = thisEnemy.IndexOf("*");
                                    tempNpcParam = thisEnemy.Substring(0, tempnpcint);
                                    tempThinkId = thisEnemy.Substring(tempnpcint + 1, thisEnemy.LastIndexOf("*") - tempnpcint - 1);
                                    modelName = thisEnemy.Substring(thisEnemy.LastIndexOf("*") + 1, 5);
                                    tempNpcParamInt = int.Parse(tempNpcParam);
                                    tempThinkIdInt = int.Parse(tempThinkId);

                                    addCounter++;
                                    if (addCounter >= combinedBossList.Count)
                                    {
                                        //enemyDataRandomized.Add(secondaryBosses[random]);
                                        addCounter = 0;
                                    }
                                }

                                string npc = tempGUY.Parts.Enemies[i].NPCParamID.ToString();
                                string think = tempGUY.Parts.Enemies[i].ThinkParamID.ToString();
                                string mo = tempGUY.Parts.Enemies[i].ModelName;
                                string entityID = tempGUY.Parts.Enemies[i].EntityID.ToString();

                                tempGUY.Parts.Enemies[i].NPCParamID = tempNpcParamInt;
                                tempGUY.Parts.Enemies[i].ThinkParamID = tempThinkIdInt;
                                tempGUY.Parts.Enemies[i].ModelName = modelName;
                                if (tempNpcParamInt == 507200)
                                {
                                    tempGUY.Parts.Enemies[i].ThinkParamID = 507200;
                                }
                                //Emissary fix
                                if (tempGUY.Parts.Enemies[i].Name == "c2500_0000")
                                {
                                    for (int k = 0; k < tempGUY.Parts.Enemies.Count; k++)
                                    {
                                        if (tempGUY.Parts.Enemies[k].Name == "c2570_0001")
                                        {
                                            tempGUY.Parts.Enemies[k].NPCParamID = tempNpcParamInt;
                                            tempGUY.Parts.Enemies[k].ThinkParamID = tempThinkIdInt;
                                            tempGUY.Parts.Enemies[k].ModelName = modelName;
                                        }
                                    }
                                }
                                //wet nurse fix
                                if (tempGUY.Parts.Enemies[i].Name == "c5510_0000")
                                {
                                    for (int k = 0; k < tempGUY.Parts.Enemies.Count; k++)
                                    {
                                        if (tempGUY.Parts.Enemies[k].Name == "c5510_0001")
                                        {
                                            tempGUY.Parts.Enemies[k].NPCParamID = tempNpcParamInt;
                                            tempGUY.Parts.Enemies[k].ThinkParamID = tempThinkIdInt;
                                            tempGUY.Parts.Enemies[k].ModelName = modelName;
                                        }
                                        if (tempGUY.Parts.Enemies[k].Name == "c5510_0002")
                                        {
                                            tempGUY.Parts.Enemies[k].NPCParamID = tempNpcParamInt;
                                            tempGUY.Parts.Enemies[k].ThinkParamID = tempThinkIdInt;
                                            tempGUY.Parts.Enemies[k].ModelName = modelName;
                                        }
                                    }
                                }
                                //maria fix
                                if (tempGUY.Parts.Enemies[i].Name == "c4520_0002")
                                {
                                    for (int k = 0; k < tempGUY.Parts.Enemies.Count; k++)
                                    {
                                        if (tempGUY.Parts.Enemies[k].Name == "c4520_0000")
                                        {
                                            tempGUY.Parts.Enemies[k].NPCParamID = tempNpcParamInt;
                                            tempGUY.Parts.Enemies[k].ThinkParamID = tempThinkIdInt;
                                            tempGUY.Parts.Enemies[k].ModelName = modelName;
                                        }
                                    }
                                }
                                //living failures fix
                                if (tempGUY.Parts.Enemies[i].Name == "c4030_0004")
                                {
                                    for (int k = 0; k < tempGUY.Parts.Enemies.Count; k++)
                                    {
                                        if (tempGUY.Parts.Enemies[k].Name == "c4030_0000")
                                        {
                                            tempGUY.Parts.Enemies[k].NPCParamID = tempNpcParamInt;
                                            tempGUY.Parts.Enemies[k].ThinkParamID = tempThinkIdInt;
                                            tempGUY.Parts.Enemies[k].ModelName = modelName;
                                        }
                                    }
                                }



                                if (logging)
                                    using (StreamWriter writetext = File.AppendText(bossLogFilePath))
                                    {
                                        writetext.WriteLine(bossCount);
                                        writetext.WriteLine(currentMap);
                                        writetext.WriteLine(i + " Number in map list");
                                        writetext.WriteLine("Old Boss");
                                        writetext.WriteLine(thisnpc + " npcParam");
                                        writetext.WriteLine(thisthink + " thinkID");
                                        writetext.WriteLine(thismo + " model");
                                        writetext.WriteLine(thisentityID);

                                        writetext.WriteLine("New Boss");
                                        writetext.WriteLine(tempNpcParam + " npcParam");
                                        writetext.WriteLine(tempThinkId + " thinkID");
                                        writetext.WriteLine(modelName + " new model");

                                        combinedBossList.RemoveAt(random);
                                        writetext.WriteLine("Enemy Pool Count: " + combinedBossList.Count);

                                        for (int m = combinedBossList.Count - 1; m >= 0; m--)
                                        {
                                            if (combinedBossList[m].Contains("*" + modelName) && !combinedBossList[m].Contains("*4510"))
                                            {
                                                combinedBossList.RemoveAt(m);
                                            }
                                        }

                                        writetext.WriteLine("New count after removing similar bosses " + combinedBossList.Count + Environment.NewLine + Environment.NewLine);
                                    }
                            }


                            bossCount++;
                            newNPCParam.Add(paramNumber);
                            mapsForParamChanges.Add(currentMap);
                            paramNumber--;

                        }
                    }
                }

                if (oopsAllBosses)
                {
                    for (int i = 0; i < tempGUY.Parts.Enemies.Count; i++)
                    {
                        changeData = false;

                        for (int j = 0; j < bossList.Count; j++)
                        {
                            if (tempGUY.Parts.Enemies[i].Name.Contains(bossList[j]))
                            {
                                if (tempGUY.Parts.Enemies[i].NPCParamID == 250060)
                                {
                                    changeData = false;
                                }
                                else if (tempGUY.Parts.Enemies[i].NPCParamID == 250070)
                                {
                                    changeData = false;
                                }
                                else if (tempGUY.Parts.Enemies[i].NPCParamID == 250090)
                                {
                                    changeData = false;
                                }
                                else if (tempGUY.Parts.Enemies[i].NPCParamID == 212600)
                                {
                                    changeData = false;
                                }
                                else if (tempGUY.Parts.Enemies[i].NPCParamID == 212610)
                                {
                                    changeData = false;
                                }
                                else if (tempGUY.Parts.Enemies[i].NPCParamID == 212620)
                                {
                                    changeData = false;
                                }
                                else if (tempGUY.Parts.Enemies[i].EntityID == -1)
                                {
                                    changeData = false;
                                }
                                else if (tempGUY.Parts.Enemies[i].NPCParamID == 0 || tempGUY.Parts.Enemies[i].NPCParamID == -1)
                                {
                                    changeData = false;
                                }
                                else
                                {
                                    if (currentMap.Contains("m29"))
                                    {
                                        changeData = false;

                                        for (int k = 0; k < chaliceBossParams.Count; k++)
                                        {
                                            if (tempGUY.Parts.Enemies[i].NPCParamID.ToString() == chaliceBossParams[k])
                                            {
                                                changeData = true;
                                                thisnpc = tempGUY.Parts.Enemies[i].NPCParamID.ToString();
                                                thisthink = tempGUY.Parts.Enemies[i].ThinkParamID.ToString();
                                                thismo = tempGUY.Parts.Enemies[i].ModelName;
                                                thisentityID = tempGUY.Parts.Enemies[i].EntityID.ToString();
                                            }
                                        }
                                    }
                                    else
                                    {
                                        changeData = true;
                                        thisnpc = tempGUY.Parts.Enemies[i].NPCParamID.ToString();
                                        thisthink = tempGUY.Parts.Enemies[i].ThinkParamID.ToString();
                                        thismo = tempGUY.Parts.Enemies[i].ModelName;
                                        thisentityID = tempGUY.Parts.Enemies[i].EntityID.ToString();
                                    }
                                }

                                if (currentMap.Contains("34"))
                                {
                                    if (tempGUY.Parts.Enemies[i].NPCParamID == 210030)
                                    {
                                        changeData = false;
                                    }
                                }
                            }
                        }


                        if (changeData && BossListString.Count > 0)
                        {


                            int random = universalRand.Next(0, BossListString.Count);
                            string thisEnemy = BossListString[random];

                            string tempNpcParam;
                            string tempThinkId;
                            string modelName;

                            int tempNpcParamInt;
                            int tempThinkIdInt;

                            int tempnpcint = thisEnemy.IndexOf("*");
                            tempNpcParam = thisEnemy.Substring(0, tempnpcint);
                            tempThinkId = thisEnemy.Substring(tempnpcint + 1, thisEnemy.LastIndexOf("*") - tempnpcint - 1);
                            modelName = thisEnemy.Substring(thisEnemy.LastIndexOf("*") + 1, 5);

                            tempNpcParamInt = int.Parse(tempNpcParam);
                            tempThinkIdInt = int.Parse(tempThinkId);

                            tempGUY.Parts.Enemies[i].NPCParamID = tempNpcParamInt;
                            tempGUY.Parts.Enemies[i].ThinkParamID = tempThinkIdInt;
                            tempGUY.Parts.Enemies[i].ModelName = modelName;


                            if (!currentMap.Contains("m29"))
                            {

                                string npc = tempGUY.Parts.Enemies[i].NPCParamID.ToString();
                                string think = tempGUY.Parts.Enemies[i].ThinkParamID.ToString();
                                string mo = tempGUY.Parts.Enemies[i].ModelName;
                                string entityID = tempGUY.Parts.Enemies[i].EntityID.ToString();

                                tempGUY.Parts.Enemies[i].NPCParamID = tempNpcParamInt;
                                tempGUY.Parts.Enemies[i].ThinkParamID = tempThinkIdInt;
                                tempGUY.Parts.Enemies[i].ModelName = modelName;

                                if (tempNpcParamInt == 507200)
                                {
                                    tempGUY.Parts.Enemies[i].ThinkParamID = 507200;
                                }
                                //emissary fix
                                if (tempGUY.Parts.Enemies[i].Name == "c2500_0000")
                                {
                                    for (int k = 0; k < tempGUY.Parts.Enemies.Count; k++)
                                    {
                                        if (tempGUY.Parts.Enemies[k].Name == "c2570_0001")
                                        {
                                            tempGUY.Parts.Enemies[k].NPCParamID = tempNpcParamInt;
                                            tempGUY.Parts.Enemies[k].ThinkParamID = tempThinkIdInt;
                                            tempGUY.Parts.Enemies[k].ModelName = modelName;
                                        }
                                    }
                                }
                                //wet nurse fix
                                if (tempGUY.Parts.Enemies[i].Name == "c5510_0000")
                                {
                                    for (int k = 0; k < tempGUY.Parts.Enemies.Count; k++)
                                    {
                                        if (tempGUY.Parts.Enemies[k].Name == "c5510_0001")
                                        {
                                            tempGUY.Parts.Enemies[k].NPCParamID = tempNpcParamInt;
                                            tempGUY.Parts.Enemies[k].ThinkParamID = tempThinkIdInt;
                                            tempGUY.Parts.Enemies[k].ModelName = modelName;
                                        }
                                        if (tempGUY.Parts.Enemies[k].Name == "c5510_0002")
                                        {
                                            tempGUY.Parts.Enemies[k].NPCParamID = tempNpcParamInt;
                                            tempGUY.Parts.Enemies[k].ThinkParamID = tempThinkIdInt;
                                            tempGUY.Parts.Enemies[k].ModelName = modelName;
                                        }
                                    }
                                }
                                //maria fix
                                if (tempGUY.Parts.Enemies[i].Name == "c4520_0002")
                                {
                                    for (int k = 0; k < tempGUY.Parts.Enemies.Count; k++)
                                    {
                                        if (tempGUY.Parts.Enemies[k].Name == "c4520_0000")
                                        {
                                            tempGUY.Parts.Enemies[k].NPCParamID = tempNpcParamInt;
                                            tempGUY.Parts.Enemies[k].ThinkParamID = tempThinkIdInt;
                                            tempGUY.Parts.Enemies[k].ModelName = modelName;
                                        }
                                    }
                                }
                                //living failures fix
                                if (tempGUY.Parts.Enemies[i].Name == "c4030_0004")
                                {
                                    for (int k = 0; k < tempGUY.Parts.Enemies.Count; k++)
                                    {
                                        if (tempGUY.Parts.Enemies[k].Name == "c4030_0000")
                                        {
                                            tempGUY.Parts.Enemies[k].NPCParamID = tempNpcParamInt;
                                            tempGUY.Parts.Enemies[k].ThinkParamID = tempThinkIdInt;
                                            tempGUY.Parts.Enemies[k].ModelName = modelName;
                                        }
                                    }
                                }

                                if (logging)
                                    using (StreamWriter writetext = File.AppendText(bossLogFilePath))
                                    {
                                        writetext.WriteLine(bossCount);
                                        writetext.WriteLine(currentMap);
                                        writetext.WriteLine(i + " Number in map list");
                                        writetext.WriteLine("Old Boss");
                                        writetext.WriteLine(thisnpc + " npcParam");
                                        writetext.WriteLine(thisthink + " thinkID");
                                        writetext.WriteLine(thismo + " model");
                                        writetext.WriteLine(thisentityID);

                                        writetext.WriteLine("New Boss");
                                        writetext.WriteLine(tempNpcParam + " npcParam");
                                        writetext.WriteLine(tempThinkId + " thinkID");
                                        writetext.WriteLine(modelName + " new model");

                                        writetext.WriteLine("Enemy Pool Count: " + BossListString.Count + Environment.NewLine + Environment.NewLine);
                                    }
                            }
                            bossCount++;
                            newNPCParam.Add(paramNumber);
                            mapsForParamChanges.Add(currentMap);
                            paramNumber--;

                        }

                    }
                }

                tempGUY.Write(currentMap);
            }
        }

        private void RandomizeMeleeMoveset(string thisParam, List<int> numToReplace, List<int> rowsToReplace)
        {
            var paramdefs = new List<PARAMDEF>();
            var paramdefbnd = BND4.Read(paramDefPath);
            foreach (BinderFile file in paramdefbnd.Files)
            {
                var paramdef = PARAMDEF.Read(file.Bytes);
                paramdefs.Add(paramdef);
            }

            var parms = new Dictionary<string, PARAM>();
            var parambnd = BND4.Read(paramPath);
            foreach (BinderFile file in parambnd.Files)
            {
                string name = System.IO.Path.GetFileNameWithoutExtension(file.Name);
                var param = PARAM.Read(file.Bytes);

                param.ApplyParamdef(paramdefs.Find(def => def.ParamType == param.ParamType));
                parms[name] = param;
            }

            PARAM currentParam;

            currentParam = parms[thisParam];

            List<PARAM.Row> rowList = new List<PARAM.Row>();
            List<PARAM.Row> randomizedRowList = new List<PARAM.Row>();



            int numOfCells = currentParam.Rows[0].Cells.Count - 1;

            for (int i = 0; i < numOfCells; i++)
            {
                bool doFirst = false;
                for (int k = 0; k < numToReplace.Count; k++)
                {
                    if (k == i)
                    {
                        doFirst = true;
                    }
                }

                if (doFirst)
                {
                    List<PARAM.Cell> cellList = new List<PARAM.Cell>();
                    List<PARAM.Cell> randomizedCellList = new List<PARAM.Cell>();

                    for (int j = 0; j < currentParam.Rows.Count; j++)
                    {
                        bool doStuff = true;

                        for (int k = 0; k < rowsToReplace.Count; k++)
                        {
                            if (currentParam.Rows[j].ID.ToString().Contains(rowsToReplace[k].ToString()))
                            {
                                doStuff = false;
                            }
                        }

                        if (doStuff)
                        {
                            cellList.Add(currentParam.Rows[j].Cells[i]);
                        }
                    }

                    //randomize
                    while (cellList.Count > 0)
                    {
                        int randomNumber = universalRand.Next(0, cellList.Count);
                        randomizedCellList.Add(cellList[randomNumber]);

                        cellList.RemoveAt(randomNumber);
                    }

                    for (int j = 0; j < currentParam.Rows.Count; j++)
                    {
                        bool doStuff = true;

                        for (int k = 0; k < rowsToReplace.Count; k++)
                        {
                            if (currentParam.Rows[j].ID.ToString().Contains(rowsToReplace[k].ToString()))
                            {
                                doStuff = false;
                            }
                        }

                        if (doStuff)
                        {
                            int randomNumber = universalRand.Next(0, randomizedCellList.Count);
                            currentParam.Rows[j].Cells[i].Value = randomizedCellList[randomNumber].Value;
                            randomizedCellList.RemoveAt(randomNumber);
                        }
                    }
                }
            }

            foreach (BinderFile file in parambnd.Files)
            {
                string name = System.IO.Path.GetFileNameWithoutExtension(file.Name);
                if (parms.ContainsKey(name))
                    file.Bytes = parms[name].Write();
            }
            parambnd.Write(paramPath);
        }

        private void RandomizeGunMoveset(string thisParam, List<int> numToReplace, List<int> rowsToReplace)
        {
            var paramdefs = new List<PARAMDEF>();
            var paramdefbnd = BND4.Read(paramDefPath);
            foreach (BinderFile file in paramdefbnd.Files)
            {
                var paramdef = PARAMDEF.Read(file.Bytes);
                paramdefs.Add(paramdef);
            }

            var parms = new Dictionary<string, PARAM>();
            var parambnd = BND4.Read(paramPath);
            foreach (BinderFile file in parambnd.Files)
            {
                string name = System.IO.Path.GetFileNameWithoutExtension(file.Name);
                var param = PARAM.Read(file.Bytes);

                param.ApplyParamdef(paramdefs.Find(def => def.ParamType == param.ParamType));
                parms[name] = param;
            }

            PARAM currentParam;

            currentParam = parms[thisParam];

            List<PARAM.Row> rowList = new List<PARAM.Row>();
            List<PARAM.Row> randomizedRowList = new List<PARAM.Row>();



            int numOfCells = currentParam.Rows[0].Cells.Count - 1;

            for (int i = 0; i < numOfCells; i++)
            {
                bool doFirst = false;
                for (int k = 0; k < numToReplace.Count; k++)
                {
                    if (k == i)
                    {
                        doFirst = true;
                    }
                }

                if (doFirst)
                {
                    List<PARAM.Cell> cellList = new List<PARAM.Cell>();
                    List<PARAM.Cell> randomizedCellList = new List<PARAM.Cell>();

                    for (int j = 0; j < currentParam.Rows.Count; j++)
                    {
                        bool doStuff = false;

                        for (int k = 0; k < rowsToReplace.Count; k++)
                        {
                            if (currentParam.Rows[j].ID.ToString().Contains(rowsToReplace[k].ToString()))
                            {
                                doStuff = true;
                            }
                        }

                        if (doStuff)
                        {
                            cellList.Add(currentParam.Rows[j].Cells[i]);
                        }
                    }

                    //randomize
                    while (cellList.Count > 0)
                    {
                        int randomNumber = universalRand.Next(0, cellList.Count);
                        randomizedCellList.Add(cellList[randomNumber]);

                        cellList.RemoveAt(randomNumber);
                    }

                    for (int j = 0; j < currentParam.Rows.Count; j++)
                    {
                        bool doStuff = false;

                        for (int k = 0; k < rowsToReplace.Count; k++)
                        {
                            if (currentParam.Rows[j].ID.ToString().Contains(rowsToReplace[k].ToString()))
                            {
                                doStuff = true;
                            }
                        }

                        if (doStuff)
                        {
                            int randomNumber = universalRand.Next(0, randomizedCellList.Count);
                            currentParam.Rows[j].Cells[i].Value = randomizedCellList[randomNumber].Value;
                            randomizedCellList.RemoveAt(randomNumber);
                        }
                    }
                }
            }

            foreach (BinderFile file in parambnd.Files)
            {
                string name = System.IO.Path.GetFileNameWithoutExtension(file.Name);
                if (parms.ContainsKey(name))
                    file.Bytes = parms[name].Write();
            }
            parambnd.Write(paramPath);
        }

        private void RandomizeItemDrops()
        {
            /*
            string newItemLotToSet = "-1";

            

            List<string> newItemLotList = new List<string>();

            List<int> npcParamList = new List<int>();

            List<string> dontUseList = new List<string>();

            
            for (int i = 0; i < unusedList.Count; i++)
            {
                dontUseList.Add(unusedList[i]);
            }
            for(int i = 0; i < bossList.Count; i ++)
            {
                dontUseList.Add(bossList[i]);
            }

            for (int i = 0; i < mapList.Count; i++)
            {
                var tempGUY = MSBB.Read(mapList[i]);

                for (int j = 0; j < tempGUY.Parts.Enemies.Count; j++)
                {
                    bool add = true;

                    for (int h = 0; h < dontUseList.Count; h++)
                    {
                        if (tempGUY.Parts.Enemies[j].Name.Contains(dontUseList[h]))
                        {
                            add = false;
                            h = dontUseList.Count + 1;
                        }
                    }

                    if (add)
                    {
                        npcParamList.Add(tempGUY.Parts.Enemies[j].NPCParamID);
                    }
                }
            }

            List<int> uniqueNPCList = npcParamList.Distinct().ToList();
            npcParamList = new List<int>();

            for (int i = 0; i < uniqueNPCList.Count; i++)
            {
                npcParamList.Add(uniqueNPCList[i]);
            }

            var logFile = File.ReadAllLines(filePath + "\\Mod Files\\NPC Scaling File\\NPCScalingFile.txt");
            logList2 = new List<string>(logFile);

            longList2 = new List<long>();
            for (int i = 0; i < logList2.Count; i++)
            {
                longList2.Add(long.Parse(logList2[i]));
            }

            //////////////////////////////////////////////////////////////////////////


            var paramdefs = new List<PARAMDEF>();
            var paramdefbnd = BND4.Read(paramDefPath);
            foreach (BinderFile file in paramdefbnd.Files)
            {
                var paramdef = PARAMDEF.Read(file.Bytes);
                paramdefs.Add(paramdef);
            }

            var parms = new Dictionary<string, PARAM>();
            var parambnd = BND4.Read(paramPath);
            foreach (BinderFile file in parambnd.Files)
            {
                string name = Path.GetFileNameWithoutExtension(file.Name);
                var param = PARAM.Read(file.Bytes);

                param.ApplyParamdef(paramdefs.Find(def => def.ParamType == param.ParamType));
                parms[name] = param;
            }

            PARAM npcParamRow = parms["NpcParam"];

            for (int i = 0; i < npcParamRow.Rows.Count; i ++)
            {
                for(int j = 0; j < npcParamList.Count; j ++)
                {
                    if(npcParamRow.Rows[i].ID == Convert.ToInt64(npcParamList[j]))
                    {
                        if (npcParamRow.Rows[i].Cells[11].Value != null)
                        {

                            string testString = npcParamRow.Rows[i].Cells[11].Value.ToString();

                            if (testString != "-1")
                            {
                                newItemLotList.Add(npcParamRow.Rows[i].Cells[11].Value.ToString());
                            }
                            else
                            {
                                npcParamList.RemoveAt(j);
                            }
                        }
                    }
                }
            }

            for (int i = 0; i < npcParamRow.Rows.Count; i ++)
            {
                for(int j = 0; j < npcParamList.Count; j ++)
                {
                    if (npcParamRow.Rows[i].ID == npcParamList[j] && newItemLotList.Count > 0)
                    {
                        int randomNumber = rand.Next(0,newItemLotList.Count);

                        using (StreamWriter writetext = File.AppendText(randomizedItemLotPath))
                        {
                            writetext.WriteLine("Old item lot");
                            writetext.WriteLine(npcParamRow.Rows[i]);
                            writetext.WriteLine(npcParamRow.Rows[i].Cells[11].Value.ToString());
                            writetext.WriteLine("New item lot");
                            writetext.WriteLine(newItemLotList[randomNumber]);
                            writetext.WriteLine(Environment.NewLine);
                        }

                        npcParamRow.Rows[i].Cells[11].Value = Int32.Parse(newItemLotList[randomNumber]);

                        newItemLotList.RemoveAt(randomNumber);
                    }
                }
            }

            for (int i = 0; i < npcParamList.Count; i ++)
            {
                List<int> newParamList = new List<int>();

                for(int j = 0; j < longList2.Count; j ++)
                {
                    if(npcParamList[i] == longList2[j])
                    {
                        for(int k = 1; k < 30; k ++)
                        {
                            newParamList.Add(Convert.ToInt32(longList2[k]));
                            j = longList2.Count + 1;
                        }
                    }
                }

                for (int j = 0; j < npcParamRow.Rows.Count; j ++)
                {
                    if(npcParamRow.Rows[j].ID == Convert.ToInt64(npcParamList[i]))
                    {
                        newItemLotToSet = npcParamRow.Rows[j].Cells[11].Value.ToString();
                        j = npcParamRow.Rows.Count;
                    }
                }

                int stopAt31 = 0;

                for(int j = 0; j < npcParamRow.Rows.Count; j ++)
                {
                    for(int k = 0; k < newParamList.Count; k ++)
                    {
                        if(npcParamRow.Rows[j].ID == newParamList[k])
                        {
                            stopAt31++;
                            npcParamRow.Rows[j].Cells[11].Value = Int32.Parse(newItemLotToSet);
                            k = newParamList.Count + 1;
                        }
                    }

                    if(stopAt31 == 31)
                    {
                        j = npcParamRow.Rows.Count;
                    }
                }
            }

            foreach (BinderFile file in parambnd.Files)
            {
                string name = Path.GetFileNameWithoutExtension(file.Name);
                if (parms.ContainsKey(name))
                    file.Bytes = parms[name].Write();
            }
            parambnd.Write(paramPath);
            */

            var paramdefs = new List<PARAMDEF>();
            var paramdefbnd = BND4.Read(paramDefPath);
            foreach (BinderFile file in paramdefbnd.Files)
            {
                var paramdef = PARAMDEF.Read(file.Bytes);
                paramdefs.Add(paramdef);
            }

            var parms = new Dictionary<string, PARAM>();
            var parambnd = BND4.Read(paramPath);
            foreach (BinderFile file in parambnd.Files)
            {
                string name = Path.GetFileNameWithoutExtension(file.Name);
                var param = PARAM.Read(file.Bytes);

                param.ApplyParamdef(paramdefs.Find(def => def.ParamType == param.ParamType));
                parms[name] = param;
            }

            PARAM npcParamRow = parms["NpcParam"];



            List<Int32> itemLotList = new List<Int32>();

            for (int i = 0; i < npcParamRow.Rows.Count; i++)
            {
                if (npcParamRow.Rows[i].ID != 252100)
                {
                    if (npcParamRow.Rows[i].ID != 6071)
                    {
                        if (npcParamRow.Rows[i].Cells[11].Value.ToString() != "-1")
                        {
                            string tempString = npcParamRow.Rows[i].Cells[11].Value.ToString();
                            itemLotList.Add(Int32.Parse(tempString));
                        }
                        if (npcParamRow.Rows[i].Cells[12].Value.ToString() != "-1")
                        {
                            string tempString = npcParamRow.Rows[i].Cells[12].Value.ToString();
                            itemLotList.Add(Int32.Parse(tempString));
                        }
                    }
                }
            }

            for (int i = 0; i < npcParamRow.Rows.Count; i++)
            {
                if (npcParamRow.Rows[i].ID != 252100)
                {
                    if (npcParamRow.Rows[i].ID != 6071)
                    {
                        if (npcParamRow.Rows[i].Cells[11].Value.ToString() != "-1")
                        {
                            int randomNumber = universalRand.Next(0, itemLotList.Count - 1);
                            string tempString = npcParamRow.Rows[i].Cells[11].Value.ToString();

                            while (tempString == itemLotList[randomNumber].ToString())
                            {
                                randomNumber = universalRand.Next(0, itemLotList.Count - 1);
                            }

                            npcParamRow.Rows[i].Cells[11].Value = itemLotList[randomNumber];
                        }
                    }
                }
            }

            foreach (BinderFile file in parambnd.Files)
            {
                string name = Path.GetFileNameWithoutExtension(file.Name);
                if (parms.ContainsKey(name))
                    file.Bytes = parms[name].Write();
            }
            parambnd.Write(paramPath);
        }

        private void RandomizeKeys()
        {
            List<string> mapList = new List<string>();

            List<string> mapsThatByrgenwerthClassroomKeyCanBeIn = new List<string>();
            List<string> mapsThatRomFightKeyCanBeIn = new List<string>();
            List<string> mapsThatCainhurstSummonsCanBeIn = new List<string>();
            List<string> mapsThatEyePendantCanBeIn = new List<string>();

            mapList.Add("m21_01"); // Abandoned Old Workshop
            mapList.Add("m22_00"); // Hemwick Charnel Lane
            mapList.Add("m23_00"); // Old Yharnam
            mapList.Add("m24_00"); // Cathedral Ward
            mapList.Add("m24_01"); // Central Yharnam, Iosefka's Clinic
            mapList.Add("m24_02"); // Upper Cathedral Ward, Healing Church Workshop, Altar of Despair
            mapList.Add("m25_00"); // Forsaken Castle Cainhurst
            mapList.Add("m26_00"); // Nightmare of Mensis
            mapList.Add("m27_00"); // Forbidden Woods
            mapList.Add("m28_00"); // Yahar'gul, Unseen Village
            mapList.Add("m32_00"); // Byrgenwerth, Lecture Building, Moonside Lake
            mapList.Add("m33_00"); // Nightmare Frontier
            mapList.Add("m34_00"); // Hunter's Nightmare
            mapList.Add("m35_00"); // Research Hall
            mapList.Add("m36_00"); // Fishing Hamlet



            bool continueSelectingAMap = true;

            int randomNumber = 0;

            while (continueSelectingAMap)
            {
                randomNumber = universalRand.Next(0, mapList.Count);

                if (mapList[randomNumber] != "m24_02")
                {
                    continueSelectingAMap = false;
                }
            }

            string startingMap = mapList[randomNumber];

            switch (startingMap)
            {
                case "m21_01":
                    // Abandoned Old Workshop
                    break;
                case "m22_00":
                    // Hemwick Charnel Lane
                    break;
                case "m23_00":
                    // Old Yharnam
                    break;
                case "m24_00":
                    // Cathedral Ward
                    break;
                case "m24_01":
                    // Central Yharnam, Iosefka's Clinic
                    break;
                case "m25_00":
                    // Forsaken Castle Cainhurst
                    break;
                case "m26_00":
                    // Nightmare of Mensis
                    break;
                case "m27_00":
                    // Forbidden Woods
                    break;
                case "m28_00":
                    // Yahar'gul, Unseen Village
                    break;
                case "m32_00":
                    // Byrgenwerth, Lecture Building, Moonside Lake
                    break;
                case "m33_00":
                    // Nightmare Frontier
                    break;
                case "m34_00":
                    // Hunter's Nightmare
                    break;
                case "m35_00":
                    // Research Hall
                    break;
                case "m36_00":
                    // Fishing Hamlet
                    break;
            }
        }

        private void TeamTypeRando()
        {


            var paramdefs = new List<PARAMDEF>();
            var paramdefbnd = BND4.Read(paramDefPath);
            foreach (BinderFile file in paramdefbnd.Files)
            {
                var paramdef = PARAMDEF.Read(file.Bytes);
                paramdefs.Add(paramdef);
            }

            var parms = new Dictionary<string, PARAM>();
            var parambnd = BND4.Read(paramPath);
            foreach (BinderFile file in parambnd.Files)
            {
                string name = Path.GetFileNameWithoutExtension(file.Name);
                var param = PARAM.Read(file.Bytes);

                param.ApplyParamdef(paramdefs.Find(def => def.ParamType == param.ParamType));
                parms[name] = param;
            }

            PARAM currentParam = parms["NpcParam"];
            List<PARAM.Row> talkList = new List<PARAM.Row>();

            for (int i = 0; i < currentParam.Rows.Count; i++)
            {
                byte eight = 25;

                currentParam.Rows[i].Cells[100].Value = eight;
            }

            foreach (BinderFile file in parambnd.Files)
            {
                string name = Path.GetFileNameWithoutExtension(file.Name);
                if (parms.ContainsKey(name))
                    file.Bytes = parms[name].Write();
            }
            parambnd.Write(paramPath);
        }

        private void RandomizeItemLots(string currentMap)
        {
            var tempGuy = MSBB.Read(currentMap);



            int randomNumber;

            if (itemLotList.Count > 0)
            {
                for (int i = 0; i < tempGuy.Events.Treasures.Count; i++)
                {
                    bool changeKey = true;

                    for (int j = 0; j < nonoItemLots.Count; j++)
                    {
                        if (tempGuy.Events.Treasures[i].ItemLot1 == nonoItemLots[j])
                        {
                            changeKey = false;
                        }
                    }

                    if (tempGuy.Events.Treasures[i].ItemLot1 > 0 && changeKey)
                    {
                        numberOfKeyIemsRandomized++;

                        if (logging)
                            using (StreamWriter writetext = File.AppendText(randomizedItemLotPath))
                            {
                                writetext.WriteLine(currentMap);
                                writetext.WriteLine("Number of items randomized so far " + numberOfKeyIemsRandomized);
                                writetext.WriteLine("Old item lot");
                                writetext.WriteLine(tempGuy.Events.Treasures[i].ItemLot1);
                            }

                        randomNumber = universalRand.Next(0, itemLotList.Count);
                        tempGuy.Events.Treasures[i].ItemLot1 = itemLotList[randomNumber];

                        if (logging)
                            using (StreamWriter writetext = File.AppendText(randomizedItemLotPath))
                            {
                                writetext.WriteLine("New item lot");
                                writetext.WriteLine(itemLotList[randomNumber]);
                                writetext.WriteLine(Environment.NewLine);
                            }

                        itemLotList.RemoveAt(randomNumber);
                    }
                }
            }

            tempGuy.Write(currentMap);
        }
        /*
        private void CutEnemyScan()
        {
            List<string> mapStringList = new List<string>();

            List<string> modelNameList = new List<string>();
            modelNameList.Add("c1020");
            modelNameList.Add("c1040");
            modelNameList.Add("c1070");
            modelNameList.Add("c1080");
            modelNameList.Add("c2030");
            modelNameList.Add("c2060");
            modelNameList.Add("c2070");
            modelNameList.Add("c2300");
            modelNameList.Add("c2310");
            modelNameList.Add("c2800");
            modelNameList.Add("c2810");
            modelNameList.Add("c2910");
            modelNameList.Add("c3110");
            modelNameList.Add("c5045");
            modelNameList.Add("c5050");
            modelNameList.Add("c5060");
            modelNameList.Add("c5110");
            modelNameList.Add("c5410");
            modelNameList.Add("c5420");
            modelNameList.Add("c5500");
            modelNameList.Add("c5501");
            modelNameList.Add("c5502");
            modelNameList.Add("c5520");
            modelNameList.Add("c5521");
            modelNameList.Add("c5522");
            modelNameList.Add("c7000");
            modelNameList.Add("c7010");
            modelNameList.Add("c8000");
            modelNameList.Add("c8010");
            modelNameList.Add("c8020");
            modelNameList.Add("c9000");
            modelNameList.Add("c9040");
            modelNameList.Add("c9990");
            modelNameList.Add("c9991");

            string[] filePaths = Directory.GetFiles(filePath + "\\MAPS\\", "*.dcx", SearchOption.TopDirectoryOnly);

            for(int i = 0; i < filePaths.Length; i ++)
            {
                mapStringList.Add(filePaths[i]);
            }

            List<string> newMapList = new List<string>();
            List<string> newNameList = new List<string>();

            for (int i = 0; i < mapStringList.Count; i++)
            {
                var tempGuy = MSBB.Read(mapStringList[i]);
                for(int j = 0; j < tempGuy.Parts.Enemies.Count; j ++)
                {
                    for(int k = 0; k < modelNameList.Count; k ++)
                    {
                        if(tempGuy.Parts.Enemies[j].ModelName.ToString().Contains(modelNameList[k]))
                        {
                            newMapList.Add(mapStringList[i]);
                            newNameList.Add(tempGuy.Parts.Enemies[j].NPCParamID.ToString() + "*" + tempGuy.Parts.Enemies[j].ThinkParamID.ToString() + "*" + tempGuy.Parts.Enemies[j].ModelName);
                        }
                    }
                }

            }

            cutEnemyString = newNameList.Distinct().ToList();
            List<string> uniqueMapList = newMapList.Distinct().ToList();
        }*/
        private void AiSoundParamRandomizer()
        {


            var paramdefs = new List<PARAMDEF>();
            var paramdefbnd = BND4.Read(paramDefPath);
            foreach (BinderFile file in paramdefbnd.Files)
            {
                var paramdef = PARAMDEF.Read(file.Bytes);
                paramdefs.Add(paramdef);
            }

            var parms = new Dictionary<string, PARAM>();
            var parambnd = BND4.Read(paramPath);
            foreach (BinderFile file in parambnd.Files)
            {
                string name = Path.GetFileNameWithoutExtension(file.Name);
                var param = PARAM.Read(file.Bytes);

                param.ApplyParamdef(paramdefs.Find(def => def.ParamType == param.ParamType));
                parms[name] = param;
            }

            PARAM currentParam = parms["AiSoundParam"];

            float one;
            float two;
            byte three;
            byte four;
            byte five;
            byte six;
            byte seven;

            for (int i = 0; i < currentParam.Rows.Count; i++)
            {
                switch (i)
                {
                    case 0: //0
                        one = 30;
                        two = 1;
                        three = 0;
                        four = 1;
                        five = 0;
                        six = 0;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 1: //1000
                        one = 10;
                        two = 1;
                        three = 0;
                        four = 1;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 2: //1010
                        one = 40;
                        two = 2;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 3: //1020
                        one = 40;
                        two = 15;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 4: //1030
                        one = 5;
                        two = 1;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 5: //1040
                        one = 15;
                        two = 12;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 200;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 6: //1050
                        one = 40;
                        two = 3;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 1;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 7: //1060
                        one = 15;
                        two = 1;
                        three = 0;
                        four = 1;
                        five = 4;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 8: //1070
                        one = 20;
                        two = 2;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 1;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 9: //2000
                        one = 10;
                        two = 6;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 10: //2010
                        one = 0;
                        two = 3;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 20;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 11: //2020
                        one = 70;
                        two = 1;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 1;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 12: //2100
                        one = 20;
                        two = 15;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 13: //2110
                        one = 100;
                        two = 1;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 14: //2120
                        one = 80;
                        two = 5;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 30;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 15: //3000
                        one = 13;
                        two = 2;
                        three = 0;
                        four = 1;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 16: //3010
                        one = 13;
                        two = 5;
                        three = 0;
                        four = 0;
                        five = 4;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 17: //3020
                        one = 20;
                        two = 5;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 18: //4000
                        one = 15;
                        two = 1;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 19: //4001
                        one = 30;
                        two = 10;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 20: //4002
                        one = 2.5f;
                        two = 1;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 21: //4003
                        one = 7;
                        two = 2;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 22: //4010
                        one = 10;
                        two = 1;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 23: //5000
                        one = 40;
                        two = 1;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 1;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 24: //5100
                        one = 20;
                        two = 15;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 25: //5101
                        one = 10;
                        two = 1;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 26: //5102
                        one = 25;
                        two = 2;
                        three = 0;
                        four = 1;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 27: //5103
                        one = 20;
                        two = 6;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 28: //5104
                        one = 0;
                        two = 0.5f;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 29: //5105
                        one = 5;
                        two = 1;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 30: //5106
                        one = 40;
                        two = 1;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 31: //5107
                        one = 15;
                        two = 3;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 30;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 32: //5108
                        one = 10;
                        two = 4;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 33: //5109
                        one = 7;
                        two = 3;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 34: //5110
                        one = 15;
                        two = 4;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 35: //5111
                        one = 5;
                        two = 2;
                        three = 0;
                        four = 0;
                        five = 4;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 36: //5112
                        one = 15;
                        two = 0.5f;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 37: //5200
                        one = 30;
                        two = 5;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 1;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 38: //5201
                        one = 13;
                        two = 2;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 39: //5202
                        one = 15;
                        two = 0.5f;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 40: //5210
                        one = 5;
                        two = 1;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 41: //5211
                        one = 45;
                        two = 2;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 42: //5220
                        one = 20;
                        two = 1;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 43: //6000
                        one = 20;
                        two = 3;
                        three = 0;
                        four = 0;
                        five = 4;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 44: //6001
                        one = 15;
                        two = 3;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 45: //6002
                        one = 20;
                        two = 1;
                        three = 0;
                        four = 1;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 46: //6003
                        one = 18;
                        two = 1;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 47: //6004
                        one = 30;
                        two = 12;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 48: //6005
                        one = 20;
                        two = 2;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 49: //6100
                        one = 70;
                        two = 1;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 50: //6200
                        one = 10;
                        two = 1;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 200;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 51: //6300
                        one = 20;
                        two = 1;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 52: //6400
                        one = 15;
                        two = 1;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 53: //6410
                        one = 2.5f;
                        two = 2;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 54: //6500
                        one = 10;
                        two = 1;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 55: //6510
                        one = 30;
                        two = 1;
                        three = 0;
                        four = 1;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 56: //6520
                        one = 15;
                        two = 1;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 57: //102000
                        one = 40;
                        two = 1;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 58: //114000
                        one = 70;
                        two = 5;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 20;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 59: //116000
                        one = 13;
                        two = 4;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 60: //124000
                        one = 100;
                        two = 2;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;

                    case 61: //301000
                        one = 20;
                        two = 1;
                        three = 0;
                        four = 0;
                        five = 0;
                        six = 100;
                        seven = 0;
                        currentParam.Rows[i].Cells[0].Value = one;
                        currentParam.Rows[i].Cells[1].Value = two;
                        currentParam.Rows[i].Cells[2].Value = three;
                        currentParam.Rows[i].Cells[3].Value = four;
                        currentParam.Rows[i].Cells[4].Value = five;
                        currentParam.Rows[i].Cells[5].Value = six;
                        currentParam.Rows[i].Cells[6].Value = seven;
                        break;
                    default:
                        Console.WriteLine("Default case");
                        break;
                }
            }

            if (logging)
                using (StreamWriter writetext = File.AppendText(dummyFilePath))
                {
                    writetext.WriteLine("AiSoundParam");

                    for (int i = 0; i < currentParam.Rows.Count; i++)
                    {
                        writetext.WriteLine("ID = " + currentParam.Rows[i].ID.ToString());

                        for (int j = 0; j < currentParam.Rows[i].Cells.Count; j++)
                        {
                            writetext.WriteLine(currentParam.Rows[i].Cells[j]);
                        }
                        writetext.WriteLine("");
                    }
                }

            foreach (BinderFile file in parambnd.Files)
            {
                string name = Path.GetFileNameWithoutExtension(file.Name);
                if (parms.ContainsKey(name))
                    file.Bytes = parms[name].Write();
            }
            parambnd.Write(paramPath);
        }

        private void DecalRandomizer()
        {


            var paramdefs = new List<PARAMDEF>();
            var paramdefbnd = BND4.Read(paramDefPath);
            foreach (BinderFile file in paramdefbnd.Files)
            {
                var paramdef = PARAMDEF.Read(file.Bytes);
                paramdefs.Add(paramdef);
            }

            var parms = new Dictionary<string, PARAM>();
            var parambnd = BND4.Read(paramPath);
            foreach (BinderFile file in parambnd.Files)
            {
                string name = Path.GetFileNameWithoutExtension(file.Name);
                var param = PARAM.Read(file.Bytes);

                param.ApplyParamdef(paramdefs.Find(def => def.ParamType == param.ParamType));
                parms[name] = param;
            }

            PARAM currentParam = parms["DecalParam"];

            List<string> decalList = new List<string>();

            masterList = new List<long>();
            RandList = new List<long>();
            cellList = new List<PARAM.Cell>();
            cellListString = new List<string>();

            paramList = new List<PARAM.Cell>();
            randParamList = new List<PARAM.Cell>();

            listOfCellLists = new List<List<PARAM.Cell>>();

            for (int i = 0; i < currentParam.Rows.Count; i++)
            {
                cellList = new List<PARAM.Cell>();
                cellList = currentParam.Rows[i].Cells.ToList();
                listOfCellLists.Add(cellList);
            }

            for (int i = 0; i < listOfCellLists[0].Count; i++)
            {
                paramList = new List<PARAM.Cell>();
                randParamList = new List<PARAM.Cell>();

                for (int j = 0; j < listOfCellLists.Count; j++)
                {
                    paramList.Add(listOfCellLists[j][i]);
                }

                while (paramList.Count > 0)
                {
                    int index = universalRand.Next(0, paramList.Count);
                    randParamList.Add(paramList[index]);
                    paramList.RemoveAt(index);
                }

                for (int j = 0; j < randParamList.Count; j++)
                {
                    string tempString = listOfCellLists[j][i].ToString();
                    int spaceInd = tempString.IndexOf(" ");
                    tempString = tempString.Substring(spaceInd + 1, tempString.Length - spaceInd - 1);
                    int equalInd = tempString.IndexOf("=");
                    tempString = tempString.Substring(0, equalInd - 1);
                    currentParam.Rows[j][tempString].Value = randParamList[j].Value;
                }
            }

            foreach (BinderFile file in parambnd.Files)
            {
                string name = Path.GetFileNameWithoutExtension(file.Name);
                if (parms.ContainsKey(name))
                    file.Bytes = parms[name].Write();
            }
            parambnd.Write(paramPath);
        }

        private void TalkRandomizer()
        {


            var paramdefs = new List<PARAMDEF>();
            var paramdefbnd = BND4.Read(paramDefPath);
            foreach (BinderFile file in paramdefbnd.Files)
            {
                var paramdef = PARAMDEF.Read(file.Bytes);
                paramdefs.Add(paramdef);
            }

            var parms = new Dictionary<string, PARAM>();
            var parambnd = BND4.Read(paramPath);
            foreach (BinderFile file in parambnd.Files)
            {
                string name = Path.GetFileNameWithoutExtension(file.Name);
                var param = PARAM.Read(file.Bytes);

                param.ApplyParamdef(paramdefs.Find(def => def.ParamType == param.ParamType));
                parms[name] = param;
            }

            PARAM currentParam = parms["TalkParam"];

            List<string> decalList = new List<string>();

            masterList = new List<long>();
            RandList = new List<long>();
            cellList = new List<PARAM.Cell>();
            cellListString = new List<string>();

            paramList = new List<PARAM.Cell>();
            randParamList = new List<PARAM.Cell>();

            listOfCellLists = new List<List<PARAM.Cell>>();

            for (int i = 0; i < currentParam.Rows.Count; i++)
            {
                cellList = new List<PARAM.Cell>();
                cellList = currentParam.Rows[i].Cells.ToList();
                listOfCellLists.Add(cellList);
            }

            for (int i = 0; i < listOfCellLists[0].Count; i++)
            {
                paramList = new List<PARAM.Cell>();
                randParamList = new List<PARAM.Cell>();

                for (int j = 0; j < listOfCellLists.Count; j++)
                {
                    paramList.Add(listOfCellLists[j][i]);
                }

                while (paramList.Count > 0)
                {
                    int index = universalRand.Next(0, paramList.Count);
                    randParamList.Add(paramList[index]);
                    paramList.RemoveAt(index);
                }

                for (int j = 0; j < randParamList.Count; j++)
                {
                    string tempString = listOfCellLists[j][i].ToString();
                    int spaceInd = tempString.IndexOf(" ");
                    tempString = tempString.Substring(spaceInd + 1, tempString.Length - spaceInd - 1);
                    int equalInd = tempString.IndexOf("=");
                    tempString = tempString.Substring(0, equalInd - 1);
                    currentParam.Rows[j][tempString].Value = randParamList[j].Value;
                }
            }

            foreach (BinderFile file in parambnd.Files)
            {
                string name = Path.GetFileNameWithoutExtension(file.Name);
                if (parms.ContainsKey(name))
                    file.Bytes = parms[name].Write();
            }
            parambnd.Write(paramPath);
        }

        private void BulletRandomizer()
        {

            List<string> IDlist = new List<string>();

            /*
            IDlist.Add("461");
            IDlist.Add("470");
            IDlist.Add("471");
            IDlist.Add("500");
            IDlist.Add("501");
            IDlist.Add("502");
            IDlist.Add("503");
            IDlist.Add("504");
            IDlist.Add("510");
            IDlist.Add("511");
            IDlist.Add("512");
            IDlist.Add("513");
            IDlist.Add("514");
            IDlist.Add("520");
            IDlist.Add("521");
            IDlist.Add("522");
            IDlist.Add("523");
            IDlist.Add("524");
            IDlist.Add("530");
            IDlist.Add("531");
            IDlist.Add("532");
            IDlist.Add("533");
            IDlist.Add("534");
            IDlist.Add("540");
            IDlist.Add("541");
            IDlist.Add("542");
            IDlist.Add("543");
            IDlist.Add("544");
            IDlist.Add("550");
            IDlist.Add("551");
            IDlist.Add("552");
            IDlist.Add("553");
            IDlist.Add("554");
            IDlist.Add("600");
            IDlist.Add("601");
            IDlist.Add("602");
            IDlist.Add("610");
            IDlist.Add("611");
            IDlist.Add("612");
            IDlist.Add("620");
            IDlist.Add("621");
            IDlist.Add("622");
            IDlist.Add("630");
            IDlist.Add("631");
            IDlist.Add("632");
            IDlist.Add("700");
            IDlist.Add("701");
            IDlist.Add("710");
            IDlist.Add("711");
            IDlist.Add("720");
            IDlist.Add("721");
            IDlist.Add("722");
            IDlist.Add("723");
            IDlist.Add("730");
            IDlist.Add("731");
            IDlist.Add("732");
            IDlist.Add("740");
            IDlist.Add("750");
            IDlist.Add("800");
            IDlist.Add("801");
            IDlist.Add("802");
            IDlist.Add("900");
            IDlist.Add("901");
            IDlist.Add("902");
            IDlist.Add("903");
            IDlist.Add("904");
            IDlist.Add("960");
            IDlist.Add("1101");
            IDlist.Add("1111");
            IDlist.Add("1121");
            IDlist.Add("1131");
            IDlist.Add("1141");
            IDlist.Add("1151");
            IDlist.Add("1160");
            IDlist.Add("25401");
            IDlist.Add("25402");
            IDlist.Add("35000");
            IDlist.Add("35001");
            IDlist.Add("45011");
            IDlist.Add("51400");
            IDlist.Add("52310");
            IDlist.Add("53901");
            IDlist.Add("54003");
            IDlist.Add("54010");
            IDlist.Add("54013");
            IDlist.Add("102050");
            IDlist.Add("102051");
            IDlist.Add("102052");
            IDlist.Add("102053");
            IDlist.Add("102054");
            IDlist.Add("102059");
            IDlist.Add("102090");
            IDlist.Add("102091");
            IDlist.Add("102092");
            IDlist.Add("102093");
            IDlist.Add("102094");
            IDlist.Add("102095");
            IDlist.Add("102096");
            IDlist.Add("102097");
            IDlist.Add("102098");
            IDlist.Add("102099");
            IDlist.Add("113060");
            IDlist.Add("113070");
            IDlist.Add("201100");
            IDlist.Add("201101");
            IDlist.Add("201102");
            IDlist.Add("201103");
            IDlist.Add("201104");
            IDlist.Add("212360");
            IDlist.Add("212361");
            IDlist.Add("212362");
            IDlist.Add("216030");
            IDlist.Add("216040");
            IDlist.Add("216050");
            IDlist.Add("216060");
            IDlist.Add("216110");
            IDlist.Add("216130");
            IDlist.Add("216150");
            IDlist.Add("216151");
            IDlist.Add("216152");
            IDlist.Add("216160");
            IDlist.Add("216161");
            IDlist.Add("216170");
            IDlist.Add("216171");
            IDlist.Add("216180");
            IDlist.Add("216190");
            IDlist.Add("216191");
            IDlist.Add("216200");
            IDlist.Add("216210");
            IDlist.Add("216211");
            IDlist.Add("232010");
            IDlist.Add("232020");
            IDlist.Add("232021");
            IDlist.Add("232022");
            IDlist.Add("232030");
            IDlist.Add("253000");
            IDlist.Add("253001");
            IDlist.Add("253005");
            IDlist.Add("253006");
            IDlist.Add("256100");
            IDlist.Add("256101");
            IDlist.Add("256102");
            IDlist.Add("256103");
            IDlist.Add("260050");
            IDlist.Add("260055");
            IDlist.Add("263050");
            IDlist.Add("263051");
            IDlist.Add("263052");
            IDlist.Add("263053");
            IDlist.Add("263054");
            IDlist.Add("263059");
            IDlist.Add("263072");
            IDlist.Add("263090");
            IDlist.Add("263091");
            IDlist.Add("263092");
            IDlist.Add("263093");
            IDlist.Add("263094");
            IDlist.Add("263095");
            IDlist.Add("263096");
            IDlist.Add("263097");
            IDlist.Add("263098");
            IDlist.Add("263099");
            IDlist.Add("303280");
            IDlist.Add("304028");
            IDlist.Add("304029");
            IDlist.Add("306000");
            IDlist.Add("306001");
            IDlist.Add("306005");
            IDlist.Add("306035");
            IDlist.Add("306036");
            IDlist.Add("306100");
            IDlist.Add("311200");
            IDlist.Add("312000");
            IDlist.Add("312001");
            IDlist.Add("313021");
            IDlist.Add("313022");
            IDlist.Add("313023");
            IDlist.Add("313024");
            IDlist.Add("313065");
            IDlist.Add("313100");
            IDlist.Add("313701");
            IDlist.Add("313110");
            IDlist.Add("313112");
            IDlist.Add("313120");
            IDlist.Add("313122");
            IDlist.Add("313130");
            IDlist.Add("313132");
            IDlist.Add("400950");
            IDlist.Add("400951");
            IDlist.Add("401500");
            IDlist.Add("401510");
            IDlist.Add("405400");
            IDlist.Add("405401");
            IDlist.Add("405402");
            IDlist.Add("405410");
            IDlist.Add("405411");
            IDlist.Add("405412");
            IDlist.Add("405415");
            IDlist.Add("405416");
            IDlist.Add("405417");
            IDlist.Add("405418");
            IDlist.Add("405420");
            IDlist.Add("405425");
            IDlist.Add("405426");
            IDlist.Add("405427");
            IDlist.Add("405428");
            IDlist.Add("405500");
            IDlist.Add("405501");
            IDlist.Add("405502");
            IDlist.Add("405503");
            IDlist.Add("405504");
            IDlist.Add("405505");
            IDlist.Add("405506");
            IDlist.Add("405507");
            IDlist.Add("405508");
            IDlist.Add("405510");
            IDlist.Add("405511");
            IDlist.Add("405512");
            IDlist.Add("405513");
            IDlist.Add("405514");
            IDlist.Add("405515");
            IDlist.Add("405516");
            IDlist.Add("405517");
            IDlist.Add("405518");
            IDlist.Add("405520");
            IDlist.Add("405521");
            IDlist.Add("405522");
            IDlist.Add("405523");
            IDlist.Add("405524");
            IDlist.Add("405525");
            IDlist.Add("405526");
            IDlist.Add("405527");
            IDlist.Add("405528");
            IDlist.Add("405590");
            IDlist.Add("405591");
            IDlist.Add("450818");
            IDlist.Add("450996");
            IDlist.Add("450997");
            IDlist.Add("452390");
            IDlist.Add("453901");
            IDlist.Add("453902");
            IDlist.Add("454140");
            IDlist.Add("454341");
            IDlist.Add("507600");
            IDlist.Add("507601");
            IDlist.Add("507602");
            IDlist.Add("507603");
            IDlist.Add("507650");
            IDlist.Add("507651");
            IDlist.Add("507652");
            IDlist.Add("507653");
            IDlist.Add("507660");
            IDlist.Add("507661");
            IDlist.Add("507662");
            IDlist.Add("507663");
            IDlist.Add("507670");
            IDlist.Add("510002");
            IDlist.Add("510003");
            IDlist.Add("510004");
            IDlist.Add("510005");
            IDlist.Add("510010");
            IDlist.Add("510011");
            IDlist.Add("510012");
            IDlist.Add("510013");
            IDlist.Add("510020");
            IDlist.Add("700009");
            IDlist.Add("710000");
            IDlist.Add("750200");
            IDlist.Add("750210");
            IDlist.Add("750701");
            IDlist.Add("2400000");
            IDlist.Add("2400001");
            IDlist.Add("2400002");
            IDlist.Add("2400010");
            IDlist.Add("2400011");
            IDlist.Add("2400012");
            IDlist.Add("2400013");
            IDlist.Add("2400015");
            IDlist.Add("2400020");
            IDlist.Add("2400026");
            IDlist.Add("2400099");
            IDlist.Add("2400122");
            IDlist.Add("2700005");
            IDlist.Add("2700006");
            IDlist.Add("2900561");
            IDlist.Add("2900581");
            IDlist.Add("2900600");
            IDlist.Add("2900602");
            IDlist.Add("2900610");
            IDlist.Add("2900612");
            IDlist.Add("2900620");
            IDlist.Add("2900622");
            IDlist.Add("2900630");
            IDlist.Add("2900632");
            IDlist.Add("2900700");
            IDlist.Add("2900702");
            IDlist.Add("2900710");
            IDlist.Add("2900720");
            IDlist.Add("2900730");
            IDlist.Add("3500000");
            IDlist.Add("3500002");
            */
            var paramdefs = new List<PARAMDEF>();
            var paramdefbnd = BND4.Read(paramDefPath);
            foreach (BinderFile file in paramdefbnd.Files)
            {
                var paramdef = PARAMDEF.Read(file.Bytes);
                paramdefs.Add(paramdef);
            }

            var parms = new Dictionary<string, PARAM>();
            var parambnd = BND4.Read(paramPath);
            foreach (BinderFile file in parambnd.Files)
            {
                string name = Path.GetFileNameWithoutExtension(file.Name);
                var param = PARAM.Read(file.Bytes);

                param.ApplyParamdef(paramdefs.Find(def => def.ParamType == param.ParamType));
                parms[name] = param;
            }

            PARAM currentParam = parms["Bullet"];
            List<PARAM.Row> bulletList = new List<PARAM.Row>();
            /*
            for(int i = 0; i < currentParam.Rows.Count; i ++)
            {
                for(int j = 0; j < IDlist.Count; j ++)
                {
                    if(!currentParam.Rows[i].ID.ToString().Contains(IDlist[j]))
                    {
                        bulletList.Add(currentParam.Rows[i]);
                    }
                }
            }

            for (int i = 0; i < currentParam.Rows.Count; i ++)
            {
                for (int j = 0; j < IDlist.Count; j++)
                {
                    if (!currentParam.Rows[i].ID.ToString().Contains(IDlist[j]))
                    {
                        int randomNumber = rand.Next(0, bulletList.Count - 1);
                        currentParam.Rows[i] = bulletList[randomNumber];
                    }
                }
            }*/

            List<PARAM.Row> canonList = new List<PARAM.Row>();
            List<PARAM.Row> gunList = new List<PARAM.Row>();

            for (int i = 0; i < currentParam.Rows.Count; i++)
            {
                if (currentParam.Rows[i].ID.ToString() == "101")
                {
                    canonList.Add(currentParam.Rows[i]);
                }
                if (currentParam.Rows[i].ID.ToString() == "721")
                {
                    canonList.Add(currentParam.Rows[i]);
                }
                if (currentParam.Rows[i].ID.ToString() == "722")
                {
                    canonList.Add(currentParam.Rows[i]);
                }
                if (currentParam.Rows[i].ID.ToString() == "723")
                {
                    canonList.Add(currentParam.Rows[i]);
                }
                if (currentParam.Rows[i].ID.ToString() == "730")
                {
                    canonList.Add(currentParam.Rows[i]);
                }
                if (currentParam.Rows[i].ID.ToString() == "731")
                {
                    canonList.Add(currentParam.Rows[i]);
                }
                if (currentParam.Rows[i].ID.ToString() == "732")
                {
                    canonList.Add(currentParam.Rows[i]);
                }
            }

            for (int i = 0; i < currentParam.Rows.Count; i++)
            {

                if (currentParam.Rows[i].ID.ToString() == "504")
                {
                    currentParam.Rows[i] = canonList[0];
                }
                if (currentParam.Rows[i].ID.ToString() == "514")
                {
                    currentParam.Rows[i] = canonList[0];
                }
                if (currentParam.Rows[i].ID.ToString() == "710001")
                {
                    currentParam.Rows[i] = canonList[0];
                }
            }


            foreach (BinderFile file in parambnd.Files)
            {
                string name = Path.GetFileNameWithoutExtension(file.Name);
                if (parms.ContainsKey(name))
                    file.Bytes = parms[name].Write();
            }
            parambnd.Write(paramPath);
        }

        private void VfxRandomizer(string paramString)
        {


            var paramdefs = new List<PARAMDEF>();
            var paramdefbnd = BND4.Read(paramDefPath);
            foreach (BinderFile file in paramdefbnd.Files)
            {
                var paramdef = PARAMDEF.Read(file.Bytes);
                paramdefs.Add(paramdef);
            }

            var parms = new Dictionary<string, PARAM>();
            var parambnd = BND4.Read(paramPath);
            foreach (BinderFile file in parambnd.Files)
            {
                string name = Path.GetFileNameWithoutExtension(file.Name);
                var param = PARAM.Read(file.Bytes);

                param.ApplyParamdef(paramdefs.Find(def => def.ParamType == param.ParamType));
                parms[name] = param;
            }

            PARAM currentParam = parms[paramString];


            masterList = new List<long>();
            RandList = new List<long>();
            cellList = new List<PARAM.Cell>();
            cellListString = new List<string>();

            paramList = new List<PARAM.Cell>();
            randParamList = new List<PARAM.Cell>();

            listOfCellLists = new List<List<PARAM.Cell>>();

            for (int i = 0; i < currentParam.Rows.Count; i++)
            {
                cellList = new List<PARAM.Cell>();
                cellList = currentParam.Rows[i].Cells.ToList();
                listOfCellLists.Add(cellList);
            }

            for (int i = 0; i < listOfCellLists[0].Count; i++)
            {
                paramList = new List<PARAM.Cell>();
                randParamList = new List<PARAM.Cell>();

                for (int j = 0; j < listOfCellLists.Count; j++)
                {
                    paramList.Add(listOfCellLists[j][i]);
                }

                while (paramList.Count > 0)
                {
                    int index = universalRand.Next(0, paramList.Count); //pick a random item from the master list
                    //Console.WriteLine(paramList[index]);
                    randParamList.Add(paramList[index]); //place it at the end of the randomized list
                    paramList.RemoveAt(index);
                }

                for (int j = 0; j < randParamList.Count; j++)
                {
                    //Console.WriteLine(randParamList[j].Def);
                    //Console.WriteLine(randParamList[j].Value);
                }

                for (int j = 0; j < randParamList.Count; j++)
                {
                    string tempString = listOfCellLists[j][i].ToString();
                    //Console.WriteLine(tempString);
                    int spaceInd = tempString.IndexOf(" ");
                    tempString = tempString.Substring(spaceInd + 1, tempString.Length - spaceInd - 1);
                    //Console.WriteLine(tempString);
                    int equalInd = tempString.IndexOf("=");
                    tempString = tempString.Substring(0, equalInd - 1);
                    //Console.WriteLine(tempString);
                    //long tempLong = longList[j];


                    ////Console.WriteLine(currentParam[j].ID[tempString]);
                    currentParam.Rows[j][tempString].Value = randParamList[j].Value;
                    //currentParam.Rows[i].ID = RandList[i];
                    //Console.WriteLine(randParamList[j]);
                }

            }


            /*
            PARAM spEffectVfxParam = parms[paramString];

            List<long> columnOneList = new List<long>();
            List<long> randomizedList = new List<long>();

            for (int i = 0; i < spEffectVfxParam.Rows.Count; i++)
            {
                columnOneList.Add(spEffectVfxParam.Rows[i].ID);
            }

            while(columnOneList.Count > 0)
            {
                int randomNumber = rand.Next(0,columnOneList.Count-1);
                randomizedList.Add(columnOneList[randomNumber]);

                columnOneList.RemoveAt(randomNumber);
            }

            for(int i = 0; i < spEffectVfxParam.Rows.Count; i ++)
            {
                spEffectVfxParam.Rows[i].ID = randomizedList[i];
                spEffectVfxParam.Rows[i].Cells[0].Value = Convert.ToInt32(randomizedList[i]);
            }
            */

            foreach (BinderFile file in parambnd.Files)
            {
                string name = Path.GetFileNameWithoutExtension(file.Name);
                if (parms.ContainsKey(name))
                    file.Bytes = parms[name].Write();
            }
            parambnd.Write(paramPath);
        }

        private void GemGenParamRandomizer()
        {


            var paramdefs = new List<PARAMDEF>();
            var paramdefbnd = BND4.Read(paramDefPath);
            foreach (BinderFile file in paramdefbnd.Files)
            {
                var paramdef = PARAMDEF.Read(file.Bytes);
                paramdefs.Add(paramdef);
            }

            var parms = new Dictionary<string, PARAM>();
            var parambnd = BND4.Read(paramPath);
            foreach (BinderFile file in parambnd.Files)
            {
                string name = Path.GetFileNameWithoutExtension(file.Name);
                var param = PARAM.Read(file.Bytes);

                param.ApplyParamdef(paramdefs.Find(def => def.ParamType == param.ParamType));
                parms[name] = param;
            }

            PARAM currentParam = parms["GemGenParam"];

            List<PARAM.Row> gemGenList = new List<PARAM.Row>();
            List<string> genOne = new List<string>();
            List<string> genTwo = new List<string>();
            List<string> genThree = new List<string>();
            List<string> genFour = new List<string>();

            UInt32 one;
            Int32 two;
            float three;
            float four;

            for (int i = 0; i < currentParam.Rows.Count; i++)
            {
                if (currentParam.Rows[i].Cells[13].Value.ToString() != "-1")
                {
                    //1 x 100 0
                    //gemGenList.Add(currentParam.Rows[i]);
                    genOne.Add(currentParam.Rows[i].Cells[12].Value.ToString());
                    genTwo.Add(currentParam.Rows[i].Cells[13].Value.ToString());
                    genThree.Add(currentParam.Rows[i].Cells[14].Value.ToString());
                    genFour.Add(currentParam.Rows[i].Cells[15].Value.ToString());
                }

                if (currentParam.Rows[i].Cells[17].Value.ToString() != "-1")
                {
                    //1 x 100 0
                    //gemGenList.Add(currentParam.Rows[i]);
                    genOne.Add(currentParam.Rows[i].Cells[16].Value.ToString());
                    genTwo.Add(currentParam.Rows[i].Cells[17].Value.ToString());
                    genThree.Add(currentParam.Rows[i].Cells[18].Value.ToString());
                    genFour.Add(currentParam.Rows[i].Cells[19].Value.ToString());
                }

                if (currentParam.Rows[i].Cells[21].Value.ToString() != "-1")
                {
                    //1 x 100 0
                    //gemGenList.Add(currentParam.Rows[i]);
                    genOne.Add(currentParam.Rows[i].Cells[20].Value.ToString());
                    genTwo.Add(currentParam.Rows[i].Cells[21].Value.ToString());
                    genThree.Add(currentParam.Rows[i].Cells[22].Value.ToString());
                    genFour.Add(currentParam.Rows[i].Cells[23].Value.ToString());
                }

                if (currentParam.Rows[i].Cells[25].Value.ToString() != "-1")
                {
                    //1 x 100 0
                    //gemGenList.Add(currentParam.Rows[i]);
                    genOne.Add(currentParam.Rows[i].Cells[24].Value.ToString());
                    genTwo.Add(currentParam.Rows[i].Cells[25].Value.ToString());
                    genThree.Add(currentParam.Rows[i].Cells[26].Value.ToString());
                    genFour.Add(currentParam.Rows[i].Cells[27].Value.ToString());
                }

                if (currentParam.Rows[i].Cells[29].Value.ToString() != "-1")
                {
                    //1 x 100 0
                    //gemGenList.Add(currentParam.Rows[i]);
                    genOne.Add(currentParam.Rows[i].Cells[28].Value.ToString());
                    genTwo.Add(currentParam.Rows[i].Cells[29].Value.ToString());
                    genThree.Add(currentParam.Rows[i].Cells[30].Value.ToString());
                    genFour.Add(currentParam.Rows[i].Cells[31].Value.ToString());
                }

                if (currentParam.Rows[i].Cells[33].Value.ToString() != "-1")
                {
                    //1 x 100 0
                    //gemGenList.Add(currentParam.Rows[i]);
                    genOne.Add(currentParam.Rows[i].Cells[32].Value.ToString());
                    genTwo.Add(currentParam.Rows[i].Cells[33].Value.ToString());
                    genThree.Add(currentParam.Rows[i].Cells[34].Value.ToString());
                    genFour.Add(currentParam.Rows[i].Cells[35].Value.ToString());
                }
            }

            for (int i = 0; i < currentParam.Rows.Count; i++)
            {
                /*
                int randomNumber = rand.Next(0, gemGenList.Count - 1);

                currentParam.Rows[i].Cells[12].Value = gemGenList[randomNumber].Cells[12].Value;
                currentParam.Rows[i].Cells[13].Value = gemGenList[randomNumber].Cells[13].Value;
                currentParam.Rows[i].Cells[14].Value = gemGenList[randomNumber].Cells[14].Value;
                currentParam.Rows[i].Cells[15].Value = gemGenList[randomNumber].Cells[15].Value;

                randomNumber = rand.Next(0, gemGenList.Count - 1);

                currentParam.Rows[i].Cells[16].Value = gemGenList[randomNumber].Cells[12].Value;
                currentParam.Rows[i].Cells[17].Value = gemGenList[randomNumber].Cells[13].Value;
                currentParam.Rows[i].Cells[18].Value = gemGenList[randomNumber].Cells[14].Value;
                currentParam.Rows[i].Cells[19].Value = gemGenList[randomNumber].Cells[15].Value;

                randomNumber = rand.Next(0, gemGenList.Count - 1);

                currentParam.Rows[i].Cells[20].Value = gemGenList[randomNumber].Cells[12].Value;
                currentParam.Rows[i].Cells[21].Value = gemGenList[randomNumber].Cells[13].Value;
                currentParam.Rows[i].Cells[22].Value = gemGenList[randomNumber].Cells[14].Value;
                currentParam.Rows[i].Cells[23].Value = gemGenList[randomNumber].Cells[15].Value;
                
                randomNumber = rand.Next(0, gemGenList.Count - 1);

                currentParam.Rows[i].Cells[24].Value = gemGenList[randomNumber].Cells[12].Value;
                currentParam.Rows[i].Cells[25].Value = gemGenList[randomNumber].Cells[13].Value;
                currentParam.Rows[i].Cells[26].Value = gemGenList[randomNumber].Cells[14].Value;
                currentParam.Rows[i].Cells[26].Value = gemGenList[randomNumber].Cells[15].Value;

                randomNumber = rand.Next(0, gemGenList.Count - 1);

                currentParam.Rows[i].Cells[28].Value = gemGenList[randomNumber].Cells[12].Value;
                currentParam.Rows[i].Cells[29].Value = gemGenList[randomNumber].Cells[13].Value;
                currentParam.Rows[i].Cells[30].Value = gemGenList[randomNumber].Cells[14].Value;
                currentParam.Rows[i].Cells[31].Value = gemGenList[randomNumber].Cells[15].Value;

                randomNumber = rand.Next(0, gemGenList.Count - 1);

                currentParam.Rows[i].Cells[32].Value = gemGenList[randomNumber].Cells[12].Value;
                currentParam.Rows[i].Cells[33].Value = gemGenList[randomNumber].Cells[13].Value;
                currentParam.Rows[i].Cells[34].Value = gemGenList[randomNumber].Cells[14].Value;
                currentParam.Rows[i].Cells[35].Value = gemGenList[randomNumber].Cells[15].Value;
                */

                float randomFLoat = universalRand.Next(0, 101);
                int numOfSlots = 0;

                if (randomFLoat < 60 && randomFLoat > 0)
                {
                    numOfSlots = 1;
                }

                if (randomFLoat < 90 && randomFLoat >= 60)
                {
                    numOfSlots = 2;
                }

                if (randomFLoat < 99 && randomFLoat >= 90)
                {
                    numOfSlots = 3;
                }

                if (randomFLoat < 100 && randomFLoat >= 99)
                {
                    numOfSlots = 4;
                }

                if (randomFLoat < 101 && randomFLoat >= 100)
                {
                    numOfSlots = 5;
                }

                if (randomFLoat == 101)
                {
                    numOfSlots = 6;
                }

                if (sixGemBool)
                {
                    numOfSlots = 6;
                }

                if (threeGemBool)
                {
                    numOfSlots = 3;
                }

                if (numOfSlots >= 1)
                {
                    int randomNumber = universalRand.Next(0, genOne.Count - 1);

                    currentParam.Rows[i].Cells[12].Value = Convert.ToUInt32(genOne[randomNumber]);
                    currentParam.Rows[i].Cells[13].Value = Convert.ToInt32(genTwo[randomNumber]);
                    currentParam.Rows[i].Cells[14].Value = float.Parse(genThree[randomNumber]);
                    currentParam.Rows[i].Cells[15].Value = float.Parse(genFour[randomNumber]);
                }

                if (numOfSlots >= 2)
                {
                    int randomNumber = universalRand.Next(0, genOne.Count - 1);

                    currentParam.Rows[i].Cells[16].Value = Convert.ToUInt32(genOne[randomNumber]);
                    currentParam.Rows[i].Cells[17].Value = Convert.ToInt32(genTwo[randomNumber]);
                    currentParam.Rows[i].Cells[18].Value = float.Parse(genThree[randomNumber]);
                    currentParam.Rows[i].Cells[19].Value = float.Parse(genFour[randomNumber]);
                }

                if (numOfSlots >= 3)
                {
                    int randomNumber = universalRand.Next(0, genOne.Count - 1);

                    currentParam.Rows[i].Cells[20].Value = Convert.ToUInt32(genOne[randomNumber]);
                    currentParam.Rows[i].Cells[21].Value = Convert.ToInt32(genTwo[randomNumber]);
                    currentParam.Rows[i].Cells[22].Value = float.Parse(genThree[randomNumber]);
                    currentParam.Rows[i].Cells[23].Value = float.Parse(genFour[randomNumber]);
                }

                if (numOfSlots >= 4)
                {
                    int randomNumber = universalRand.Next(0, genOne.Count - 1);

                    currentParam.Rows[i].Cells[24].Value = Convert.ToUInt32(genOne[randomNumber]);
                    currentParam.Rows[i].Cells[25].Value = Convert.ToInt32(genTwo[randomNumber]);
                    currentParam.Rows[i].Cells[26].Value = float.Parse(genThree[randomNumber]);
                    currentParam.Rows[i].Cells[26].Value = float.Parse(genFour[randomNumber]);
                }

                if (numOfSlots >= 5)
                {
                    int randomNumber = universalRand.Next(0, genOne.Count - 1);

                    currentParam.Rows[i].Cells[28].Value = Convert.ToUInt32(genOne[randomNumber]);
                    currentParam.Rows[i].Cells[29].Value = Convert.ToInt32(genTwo[randomNumber]);
                    currentParam.Rows[i].Cells[30].Value = float.Parse(genThree[randomNumber]);
                    currentParam.Rows[i].Cells[31].Value = float.Parse(genFour[randomNumber]);
                }

                if (numOfSlots >= 6)
                {
                    int randomNumber = universalRand.Next(0, genOne.Count - 1);

                    currentParam.Rows[i].Cells[32].Value = Convert.ToUInt32(genOne[randomNumber]);
                    currentParam.Rows[i].Cells[33].Value = Convert.ToInt32(genTwo[randomNumber]);
                    currentParam.Rows[i].Cells[34].Value = float.Parse(genThree[randomNumber]);
                    currentParam.Rows[i].Cells[35].Value = float.Parse(genFour[randomNumber]);
                }
            }

            foreach (BinderFile file in parambnd.Files)
            {
                string name = Path.GetFileNameWithoutExtension(file.Name);
                if (parms.ContainsKey(name))
                    file.Bytes = parms[name].Write();
            }
            parambnd.Write(paramPath);
        }
    }
}
