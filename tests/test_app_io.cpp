// Integration of the actual app's atomic writer; uses only synthetic saves in
// the explicitly supplied scratch directory. Never pass a game save directory.
#define wWinMain SuzerainUnusedGuiEntry
#include "../src/main.cpp"
#undef wWinMain
#include <iostream>
#include <functional>
#include <thread>

int main(int argc,char** argv){
    if(argc!=2){std::cerr<<"Usage: test_app_io <new-scratch-directory>\n";return 2;}
    size_t checks=0;
    auto check=[&](bool ok,const char* text){++checks;if(!ok)throw std::runtime_error(text);};
    auto rejects=[&](const std::function<void()>& operation,const char* text){bool rejected=false;try{operation();}catch(const std::exception&){rejected=true;}check(rejected,text);};
    const fs::path root=fs::absolute(fs::u8path(argv[1]));
    try{
        if(fs::exists(root))throw std::runtime_error("Scratch directory must be new");
        if(gameRunning())throw std::runtime_error("Game is running; synthetic writer tests not started");
        fs::create_directories(root);
        auto save=[](const std::string& story,const std::string& prefix,int value){
            return "{\"currentStoryPack\":\""+story+"\",\"version\":\"test\",\"campaignName\":\"synthetic\",\"turnNo\":1,\"variables\":\"Variable={[\\\""+prefix+"Budget\\\"]="+std::to_string(value)+",[\\\"OtherMod.Keep\\\"]=17};\"}";
        };
        auto write=[](const fs::path& path,const std::string& data){std::ofstream out(path,std::ios::binary);out<<data;if(!out)throw std::runtime_error("Fixture write failed");};
        const auto riziaOld=save("StoryPack_Rizia","RiziaDLC.",5),riziaNew=save("StoryPack_Rizia","RiziaDLC.",6);
        const auto mainOld=save("StoryPack_Main","BaseGame.",7),mainNew=save("StoryPack_Main","BaseGame.",8);
        const auto riziaPath=root/"rizia.json",mainPath=root/"main.json";
        write(riziaPath,riziaOld);write(mainPath,mainOld);
        {
            SaveLock held(mainPath);
            bool busy = false;
            std::thread competing([&] {
                try { atomicSave(mainPath,mainNew,&mainOld); }
                catch (const WinFailure&) { busy = true; }
            });
            competing.join();
            check(busy && readFile(mainPath)==mainOld && !fs::exists(root/".rizia-backups"),
                  "Concurrent save was not blocked before backup/write");
        }
        const auto practiceCopy=root/"practice.json";
        check(atomicSave(practiceCopy,mainNew,nullptr).empty()&&readFile(practiceCopy)==mainNew,"Practice copy was not created");
        rejects([&]{atomicSave(mainPath,mainNew,nullptr);},"Practice save overwrote an existing file");
        check(readFile(mainPath)==mainOld&&!fs::exists(root/".rizia-backups"),"Rejected practice save changed the destination");
        rejects([&]{atomicSave(mainPath,riziaNew,&mainOld);},"Cross-story overwrite accepted");
        check(readFile(mainPath)==mainOld&&!fs::exists(root/".rizia-backups"),"Rejected cross-story overwrite changed files");
        rejects([&]{atomicSave(riziaPath,mainNew,&riziaOld);},"Cross-story overwrite with expected bytes accepted");
        check(readFile(riziaPath)==riziaOld,"Rizia changed on mismatch");
        const auto unknown=save("StoryPack_Unknown","BaseGame.",8);
        rejects([&]{atomicSave(root/"unknown.json",unknown,nullptr);},"Unknown output story accepted");
        check(!fs::exists(root/"unknown.json"),"Unknown output created file");
        std::string stale=mainOld+" ";
        rejects([&]{atomicSave(mainPath,mainNew,&stale);},"Stale opened bytes accepted");
        check(readFile(mainPath)==mainOld,"Stale rejection changed save");
        const auto copyTarget=root/"alternate-slot.json";
        const auto otherProgress=save("StoryPack_Main","BaseGame.",9);
        // Another writer creates, changes or removes the chosen slot while
        // Studio is rendering the edits from mainPath.
        write(copyTarget,mainOld);
        rejects([&]{atomicSave(copyTarget,mainNew,nullptr);},"Save As overwrote a newly created slot");
        check(readFile(copyTarget)==mainOld,"Newly created slot changed");
        const auto chosenBytes=readFile(copyTarget);
        write(copyTarget,otherProgress);
        rejects([&]{atomicSave(copyTarget,mainNew,&chosenBytes);},"Save As overwrote newer progress");
        check(readFile(copyTarget)==otherProgress,"External progress was lost");
        fs::remove(copyTarget);
        rejects([&]{atomicSave(copyTarget,mainNew,&chosenBytes);},"Save As recreated a removed slot");
        check(!fs::exists(copyTarget)&&!fs::exists(root/".rizia-backups"),"Conflicts wrote a slot or a backup");
        write(copyTarget,chosenBytes);
        const auto copyBackup=atomicSave(copyTarget,mainNew,&chosenBytes);
        check(readFile(copyTarget)==mainNew&&readFile(copyBackup)==chosenBytes&&readFile(mainPath)==mainOld,
              "An unchanged Save As destination must keep its own exact backup and leave the source alone");
        const auto mainBackup=atomicSave(mainPath,mainNew,&mainOld);
        check(readFile(mainPath)==mainNew&&readFile(mainBackup)==mainOld,"Main save/backup not byte-exact");
        const auto branchPath=root/"main.json.branch.json";
        write(branchPath, mainOld);
        const auto branchBackup=atomicSave(branchPath, mainNew, &mainOld);
        const auto backups=save_backup::list(mainPath,MAX_FILE);
        check(backups.size()==1&&backups.front().path==mainBackup,
              "A backup of a similarly named playthrough appeared under the original save");
        const auto branchBackups=save_backup::list(branchPath,MAX_FILE);
        check(branchBackups.size()==1&&branchBackups.front().path==branchBackup,
              "The branch playthrough lost its own backup");
        const auto archive=root/".rizia-backups";
        const auto dated=root/"save.20240101-010203-004.json";
        const auto datedBackup=archive/"save.20240101-010203-004.json.20261121-140512-900.77E7E288-D7A1-48E8-9EDB-0A90F638245D.bak";
        write(datedBackup,mainOld);
        const auto datedList=save_backup::list(dated,MAX_FILE);
        check(datedList.size()==1&&fileTimeText(datedList.front().path,datedList.front().timestamp)==L"2026-11-21  14:05:12",
              "A timestamp embedded in the original name was shown as the backup date");
        write(archive/"MAIN.JSON.20200101-020304-000.bak",mainOld);
        write(archive/"main.json.my-checkpoint.bak",mainOld);
        write(archive/"main.json.branch.json.my-checkpoint.bak",mainOld);
        write(archive/"main.json.bak",mainOld);
        const auto future=fs::file_time_type::clock::now()+std::chrono::hours(24);
        fs::last_write_time(archive/"main.json.my-checkpoint.bak",future);
        const auto legacy=save_backup::list(mainPath,MAX_FILE);
        check(legacy.size()==4&&legacy.front().path.filename()=="main.json.my-checkpoint.bak"&&
              legacy.back().timestamp==L"20200101-020304-000",
              "Timestamp-only and manually labeled backups should retain exact ownership");
        rejects([&]{atomicSave(mainBackup,mainNew,&mainOld);},"Existing recovery point was writable as an ordinary save");
        check(readFile(mainBackup)==mainOld&&!fs::exists(archive/".rizia-backups"),
              "Writing a recovery point changed its bytes or created nested backups");
        fs::create_directories(archive/"subfolder");
        const auto protectedCopy=archive/"subfolder"/"export.json";
        rejects([&]{atomicSave(protectedCopy,mainOld,nullptr);},"New save created inside a backup subfolder");
        check(!fs::exists(protectedCopy),"Rejected nested destination was created");
        const auto recovered=root/"recovered.json";
        check(atomicSave(recovered,readFile(mainBackup),nullptr).empty()&&readFile(recovered)==mainOld,
              "Saving a separate copy from a protected backup failed");
        const auto riziaBackup=atomicSave(riziaPath,riziaNew,&riziaOld);
        check(readFile(riziaPath)==riziaNew&&readFile(riziaBackup)==riziaOld,"Rizia save/backup not byte-exact");
        rejects([&]{atomicSave(mainPath,readFile(riziaBackup),&mainNew);},"Foreign backup restored over Main");
        check(readFile(mainPath)==mainNew,"Foreign restore changed target");
        const auto restoreBackup=atomicSave(mainPath,readFile(mainBackup),&mainNew);
        check(readFile(mainPath)==mainOld&&readFile(restoreBackup)==mainNew,"Restore or pre-restore backup not exact");
        check(readFile(mainBackup)==mainOld,"Restore mutated source backup");
        auto external=mainOld+"\n";write(mainPath,external);
        rejects([&]{atomicSave(mainPath,mainNew,&mainOld);},"Externally changed file accepted");
        check(readFile(mainPath)==external,"Conflict overwrote external change");
        const auto newPath=root/"new-main.json";
        check(atomicSave(newPath,mainNew,nullptr).empty()&&readFile(newPath)==mainNew,"Save As new path failed");
        bool temporary=false;for(const auto& entry:fs::directory_iterator(root))if(entry.path().extension()==".tmp")temporary=true;
        check(!temporary,"Temporary write file left behind");
        // Exercise patch/CSV writer on scratch only; production dialogs remain manual QA.
        const auto mainPatch=rizia::serializePatch({{"BaseGame.Budget","9"}},"StoryPack_Main");
        writePatch(root/"main.patch.txt",mainPatch);
        check(rizia::parsePatch(readFile(root/"main.patch.txt"),"StoryPack_Main").at("BaseGame.Budget")=="9","Patch writer roundtrip failed");
        const auto csvDoc=rizia::Document::parse(R"({"currentStoryPack":"StoryPack_Main","variables":"Variable={['BaseGame.GovernmentBudget']=-3,['BaseGame.-3']=-0,['BaseGame. =1+2']=2e+1,['BaseGame.Note,\"Q\"']=false}"})");
        rizia::EditSession csvSession(csvDoc);
        csvSession.applyBatch({{"BaseGame.GovernmentBudget","-2.50e-1"},{"BaseGame.Note,\"Q\"","true"}});
        activeStory=csvDoc.story;
        i18n::setLanguage(2);
        writePatch(root/"fields.csv",renderFieldCsv(csvDoc,csvSession.pending()));
        const auto csv=readFile(root/"fields.csv");
        const std::string expectedCsv="\xEF\xBB\xBF" "Campaign,Field,Label,Original,Current,Type\r\n"
            "\"StoryPack_Main\",\"BaseGame.GovernmentBudget\",\"Government budget\",\"-3\",\"-2.50e-1\",\"number\"\r\n"
            "\"StoryPack_Main\",\"BaseGame.-3\",\"'-3\",\"-0\",\"-0\",\"number\"\r\n"
            "\"StoryPack_Main\",\"BaseGame. =1+2\",\"' =1+2\",\"2e+1\",\"2e+1\",\"number\"\r\n"
            "\"StoryPack_Main\",\"BaseGame.Note,\"\"Q\"\"\",\"Note,\"\"Q\"\"\",\"false\",\"true\",\"boolean\"\r\n";
        check(csv==expectedCsv,"Field CSV must preserve signed numbers and booleans and escape risky text labels");
        const auto protectedExports=root/"campaign"/".RiZiA-BaCkUpS"/"turn-3"/"copies";
        const auto normalExports=root/"campaign"/".rizia-backups-notes"/"turn-3"/"copies";
        fs::create_directories(protectedExports);
        fs::create_directories(normalExports);
        for(const auto& sample:{root/"main.patch.txt",root/"fields.csv"}){
            const auto data=readFile(sample);
            const auto blocked=protectedExports/sample.filename();
            rejects([&]{writePatch(blocked,data);},"Export created a file inside a backup subfolder");
            check(!fs::exists(blocked),"Rejected export left a new destination");
            write(blocked,"previous export");
            rejects([&]{writePatch(blocked,data);},"Export replaced a file inside a backup subfolder");
            check(readFile(blocked)=="previous export","Rejected export changed the existing file");
            const auto allowed=normalExports/sample.filename();
            writePatch(allowed,data);
            check(readFile(allowed)==data,"Similar folder name prevented a valid export");
        }
        for(const auto& entry:fs::directory_iterator(protectedExports)){
            check(entry.path().extension()!=".tmp","Rejected export left a temporary file in the backup subfolder");
        }
        const auto gameFolder=root/"sample-game";
        const auto gameExe=gameFolder/"Suzerain.exe";
        const auto host=gameFolder/"BepInEx";
        const auto payload=root/live_setup::pluginName;
        const auto installed=host/"plugins"/"SuzerainSaveStudio"/live_setup::pluginName;
        fs::create_directories(gameFolder);
        write(payload,"synthetic plugin bytes");write(gameExe,"synthetic exe");
        rejects([&]{live_setup::install(payload,gameExe);},"Setup accepted missing mod host");
        check(!fs::exists(host/"plugins"),"Setup changed an unsupported installation");
        fs::create_directories(host/"core");fs::create_directories(host/"interop");
        write(gameFolder/"GameAssembly.dll","fixture");write(host/"core"/"BepInEx.Unity.IL2CPP.dll","fixture");
        write(host/"interop"/"Assembly-CSharp.dll","fixture");
        check(live_setup::install(payload,gameExe)&&readFile(installed)==readFile(payload),"Plugin installation");
        check(!live_setup::install(payload,gameExe),"Setup should recognize an identical installation");
        write(installed,"older user-controlled plugin");
        rejects([&]{live_setup::install(payload,gameExe);},"Setup overwrote a different plugin");
        check(readFile(installed)=="older user-controlled plugin","Setup changed the previous plugin");
        std::cout<<"PASS: "<<checks<<" synthetic app IO checks; fixtures retained at "<<root.u8string()<<'\n';
        return 0;
    }catch(const std::exception& error){std::cerr<<"FAIL after "<<checks<<" checks: "<<error.what()<<'\n';return 1;}
}
