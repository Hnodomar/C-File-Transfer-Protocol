import subprocess
import os
from time import sleep
#compile client and server, make them run, pipe output to files
#then diff those files with files that we know to be the correct output
#return number of successful diffs

class Tester:
    def addTest(self, input):
        self.tests += input
    tests = []

class GetFileTests(Tester):
    #diff filenames and response to client
    def testGetFiles(files_list):
        pass
    def runTests(self):
        for test in self.tests:
            self.testGetFiles(test)

class ClientDownloadTests(Tester):
    #diff original and downloaded files
    def testClientDownload():
        print("test")
    def runTests(self):
        for test in self.tests:
            self.testClientDownload(test)

class ClientUploadTests(Tester):
    #diff original and uploaded files
    def testClientUpload():
        print("test")
    def runTests(self):
        for test in self.tests:
            self.testClientUpload(test)

#with open('helloworld.txt', "w") as outfile:
#    subprocess.run(my_cmd, stdout=outfile)
#subprocess.call(["rm", "helloworld"])

def runMake():
    cmd_client = ["make", "client", "_OUTLOC=../tests/"]
    cmd_server = ["make", "server", "_OUTLOC=../tests/"]
    cmds = [cmd_client, cmd_server]
    for cmd in cmds:
        subprocess.Popen(cmd, cwd="../src")

def delPrograms():
   subprocess.call(["rm", "server"])
   subprocess.call(["rm", "client"])

def main():
    runMake()
    sleep(2)
    delPrograms()
    get_tests = GetFileTests()
    download_tests = ClientDownloadTests()
    upload_tests = ClientUploadTests()

if __name__ == "__main__":
    main()
