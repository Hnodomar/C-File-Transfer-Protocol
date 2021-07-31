import subprocess
import os
import operator
from time import sleep

class Tester:
    def addTest(self, input):
        self.tests.append(input)
    tests = []

class GetFileTests(Tester):
    #diff filenames and response to client
    def testGetFiles(files_list):
        pass
    def runTests(self):
        for test in self.tests:
            self.testGetFiles(test)

class ClientUploadDownloadTester(Tester):
    def __init__(self, flag):
        self.test_flag = flag
    def runTests(self):
        for test in self.tests:
            self.testClientTransfer(test)
        return self.tests_failed
    def removeOutput(self, filename):
        path = ""
        if self.test_flag == "-u":
            path = "storage/"
        elif self.test_flag == "-d":
            path = "clientfiles/"
        path += filename
        subprocess.Popen(["rm {}".format(path)], shell=True)
    def runDiff(self, path):
        filename = (path.split('/'))[1]
        cmd = ["diff storage/{} {}".format(filename, path)]
        difftask = subprocess.Popen(cmd, stdout=subprocess.PIPE, shell=True)
        difftask.communicate()
        print(difftask.returncode)
        if difftask.returncode == 0:
            print("Success!\n".format(filename))
        else:
            print("Failed!\n".format(filename))
            self.tests_failed += 1
        self.removeOutput(filename)
    def testClientTransfer(self, filename):
        cmd = ["./client", "localhost", self.test_flag, filename]
        with open('clientlog.txt', "w") as outfile:
            (subprocess.Popen(cmd, stdout=outfile)).wait()
        print("Ran test on: {}".format(filename))
        sleep(2) #wait for server to receive packets and write to file if uploading from client (problem with large files)
        self.runDiff(filename)
    test_flag = ""
    tests_failed = 0
    
def runMake():
    cmd_client = ["make", "client", "_OUTLOC=../tests/"]
    cmd_server = ["make", "server", "_OUTLOC=../tests/"]
    cmds = [cmd_client, cmd_server]
    for cmd in cmds:
        (subprocess.Popen(cmd, cwd="../src", stdout=subprocess.DEVNULL)).wait()

def startServer():
    print("\n")
    with open('serverlog.txt', "w") as outfile:
        server = subprocess.Popen("./server", stdout=outfile)
        sleep(1)
        return server

def delPrograms():
   subprocess.call(["rm", "server"])
   subprocess.call(["rm", "client"])

def addUploadTests(upload_obj):
    upload_obj.addTest("clientfiles/smallWithSpacesUpload.txt")
    upload_obj.addTest("clientfiles/smallLoremIpsumUpload.txt")
    upload_obj.addTest("clientfiles/largeLoremIpsumUpload.txt")

def addDownloadTests(download_obj):
    download_obj.addTest("storage/smallWithSpacesDownload.txt")
    download_obj.addTest("storage/smallLoremIpsumDownload.txt")
    download_obj.addTest("storage/largeLoremIpsumDownload.txt")

def performUploadTests():
    upload_tests = ClientUploadDownloadTester("-u")
    addUploadTests(upload_tests)
    upload_tests.runTests()
    return (len(upload_tests.tests), upload_tests.tests_failed)

def performDownloadTests():
    download_tests = ClientUploadDownloadTester("-d")
    addDownloadTests(download_tests)
    download_tests.runTests()
    return (len(download_tests.tests), download_tests.tests_failed)


def main():
    runMake()
    server = startServer()
    try:
        test_info = (0, 0)
        test_info = tuple(map(operator.add, test_info, performUploadTests()))
        test_info = tuple(map(operator.add, test_info, performDownloadTests()))
        print(
            "[TESTS {}]\n".format("SUCCESSFUL" if test_info[1] == 0 else "FAILED"),
            "Total tests: {}\n".format(test_info[0]),
            "Successful tests: {}\n".format(test_info[0] - test_info[1]),
            "Failed tests: {}\n".format(test_info[1])
        )
    except Exception as e:
        print(e)
    server.terminate()
    delPrograms()

if __name__ == "__main__":
    main()
