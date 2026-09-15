namespace MSB_Test
{
    public partial class MainWindow
    {
        public void SetBooleans()
        {
            // Special cases.  
            addOrphan = true;
            includeBosses = chaliceBosses = BossCheckBox.IsChecked == true;
            //if (CutEnemyBox.IsChecked == true)
            //{
            //    cutEnemyBool = true;
            //}
            //else cutEnemyBool = false;

            /*
            EasyRomBox.IsEnabled = false;
            EasyWitchesBox.IsEnabled = false;
            */

            // Checkbox booleans. 
            noScalingBool = NoScaleBox.IsChecked == true;
            meleeMoveset = MeleeMovesetBox.IsChecked == true;
            gunMoveset = GunMovesetBox.IsChecked == true;
            permaDarknessBool = PermaDarknessBox.IsChecked == true;
            bossChalices = ChaliceBossBox.IsChecked == true;
            bossHamlet = HamletBossBox.IsChecked == true;
            bossNightmare = NightmareBossBox.IsChecked == true;
            bossForbiddenWoods = WoodsBossBox.IsChecked == true;
            bossByrgenwerthLecture = ByrgenwerthBossBox.IsChecked == true;
            bossCainhurst = CainhurstBossBox.IsChecked == true;
            bossCentralYharnam = CentralYharnamBossBox.IsChecked == true;
            bossOldYharnam = OldYharnamBossBox.IsChecked == true;
            bossResearchHall = ResearchHallBossBox.IsChecked == true;
            bossYahargul = YahargulBossBox.IsChecked == true;
            bossFrontier = FrontierBossBox.IsChecked == true;
            bossMensis = MensisBossBox.IsChecked == true;
            bossUpperCathedralWard = UpperCathedralBossBox.IsChecked == true;
            bossCathedralWard = CathedralWardBossBox.IsChecked == true;
            bossHemwick = HemwickCheckBox.IsChecked == true;
            keepGuns = KeepGunsBox.IsChecked == true;
            randomizeItemLots = ArmorRandomizerCheckBox.IsChecked == true;
            includeNPCs = AddNPCS.IsChecked == true;
            randomizeEnemiesBool = RandomizeEnemiesCheck.IsChecked == true;
            oopsAllBosses = OopsAllBossesCheck.IsChecked == true;
            oopsAll = OopsAllCheck.IsChecked == true;
            insertBossesBool = InsertBosses.IsChecked == true;
            chaliceEnemies = ChaliceEnemies.IsChecked == true;
            bellMaidenBool = BellMaidenBox.IsChecked == true;
            lesserBossesBool = LesserBossesBox.IsChecked == true;
            keyItemRandomizeBool = RandomizeKeyItemsBox.IsChecked == true;
            shopBool = RandomizeShopBox.IsChecked == true;
            enemyDropBool = EnemyDropBox.IsChecked == true;
            workshopBool = WorkshopBox.IsChecked == true;
            easyMultiBossesBool = EasyMultiBossesBox.IsChecked == true;
            easyFailuresBool = EasyFailuresBox.IsChecked == true;
            startingWeaponsOnlyBool = StartingWeaponsRandomizeBox.IsChecked == true;
            vfxBool = VFXChange.IsChecked == true;
            talkBool = TalkBox.IsChecked == true;
            gemBool = GemsAndRunesBox.IsChecked == true;
            bloodBool = BloodBox.IsChecked == true;
            faceBool = FaceBox.IsChecked == true;
            teamTypeBool = AllDmgAll.IsChecked == true;
            sixGemBool = SixGemBox.IsChecked == true;
            threeGemBool = ThreeGemBox.IsChecked == true;
            customScaling = CustomScalingBox.IsChecked == true;
            easyRomBool = EasyRomBox.IsChecked == true;
            easyWitchesBool = EasyWitchesBox.IsChecked == true;
            excludeBossesBool = ExBossBox.IsChecked == true;
            excludeEnemiesBool = ExEnemyBox.IsChecked == true;

            // Non Checkbox booleans. 
            TEST.IsEnabled = false;
            TalkBox.IsEnabled = false;
            RandomizeEnemiesCheck.IsEnabled = false;
            BossCheckBox.IsEnabled = false;
            InsertBosses.IsEnabled = false;
            ChaliceBoss.IsEnabled = false;
            ChaliceEnemies.IsEnabled = false;
            ArmorRandomizerCheckBox.IsEnabled = false;
            AddNPCS.IsEnabled = false;
            OopsAllCheck.IsEnabled = false;
            OopsAllBossesCheck.IsEnabled = false;
            OopsAllStringName.IsEnabled = false;
            OopsBoss.IsEnabled = false;
            BellMaidenBox.IsEnabled = false;
            LesserBossesBox.IsEnabled = false;
            RandomizeKeyItemsBox.IsEnabled = false;
            RandomizeShopBox.IsEnabled = false;
            EnemyDropBox.IsEnabled = false;
            WorkshopBox.IsEnabled = false;

            BossDownTen.IsEnabled = false;
            BossUpOne.IsEnabled = false;
            BossUpPointOne.IsEnabled = false;
            BossUpTen.IsEnabled = false;
            BossUpOne_Copy.IsEnabled = false;
            BossUpPointOne_Copy.IsEnabled = false;

            HCDT.IsEnabled = false;
            HCDO.IsEnabled = false;
            HCDPO.IsEnabled = false;
            HCUPO.IsEnabled = false;
            HCUO.IsEnabled = false;
            HCUT.IsEnabled = false;

            CWDT.IsEnabled = false;
            CWDO.IsEnabled = false;
            CWDPO.IsEnabled = false;
            CWUPO.IsEnabled = false;
            CWUO.IsEnabled = false;
            CWUT.IsEnabled = false;

            UCDT.IsEnabled = false;
            UCDO.IsEnabled = false;
            UCDPO.IsEnabled = false;
            UCUPO.IsEnabled = false;
            UCUO.IsEnabled = false;
            UCUT.IsEnabled = false;

            MDT.IsEnabled = false;
            MDO.IsEnabled = false;
            MDPO.IsEnabled = false;
            MUPO.IsEnabled = false;
            MUO.IsEnabled = false;
            MUT.IsEnabled = false;

            YDT.IsEnabled = false;
            YDO.IsEnabled = false;
            YDPO.IsEnabled = false;
            YUPO.IsEnabled = false;
            YUO.IsEnabled = false;
            YUT.IsEnabled = false;

            FDT.IsEnabled = false;
            FDO.IsEnabled = false;
            FDPO.IsEnabled = false;
            FUPO.IsEnabled = false;
            FUO.IsEnabled = false;
            FUT.IsEnabled = false;

            OYDT.IsEnabled = false;
            OYDO.IsEnabled = false;
            OYDPO.IsEnabled = false;
            OYUPO.IsEnabled = false;
            OYUO.IsEnabled = false;
            OYUT.IsEnabled = false;

            CYDT.IsEnabled = false;
            CYDO.IsEnabled = false;
            CYDPO.IsEnabled = false;
            CYUPO.IsEnabled = false;
            CYUO.IsEnabled = false;
            CYUT.IsEnabled = false;

            CDT.IsEnabled = false;
            CDO.IsEnabled = false;
            CDPO.IsEnabled = false;
            CUPO.IsEnabled = false;
            CUO.IsEnabled = false;
            CUT.IsEnabled = false;

            WDT.IsEnabled = false;
            WDO.IsEnabled = false;
            WDPO.IsEnabled = false;
            WUPO.IsEnabled = false;
            WUO.IsEnabled = false;
            WUT.IsEnabled = false;

            BDT.IsEnabled = false;
            BDO.IsEnabled = false;
            BDPO.IsEnabled = false;
            BUPO.IsEnabled = false;
            BUO.IsEnabled = false;
            BUT.IsEnabled = false;

            HDT.IsEnabled = false;
            HDO.IsEnabled = false;
            HDPO.IsEnabled = false;
            HUPO.IsEnabled = false;
            HUO.IsEnabled = false;
            HUT.IsEnabled = false;

            FHDT.IsEnabled = false;
            FHDO.IsEnabled = false;
            FHDPO.IsEnabled = false;
            FHUPO.IsEnabled = false;
            FHUO.IsEnabled = false;
            FHUT.IsEnabled = false;

            RDT.IsEnabled = false;
            RDO.IsEnabled = false;
            RDPO.IsEnabled = false;
            RUPO.IsEnabled = false;
            RUO.IsEnabled = false;
            RUT.IsEnabled = false;

            ChaliceUpOne.IsEnabled = false;
            ChaliceUpPointOne.IsEnabled = false;
            ChaliceUpTen.IsEnabled = false;
            ChaliceUpOne_Copy.IsEnabled = false;
            ChaliceUpPointOne_Copy.IsEnabled = false;
            ChaliceDownTen.IsEnabled = false;
        }
    }
}
