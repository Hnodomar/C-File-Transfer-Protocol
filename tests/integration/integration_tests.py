import subprocess
import operator
from os import walk
from time import sleep

class FileTransferTester:
    def __init__(self, flag, dir):
        self.tests = []
        self.__test_flag = flag
        self.__baseline_dir = dir

    def addTest(self, input):
        self.tests.append(input)

    def runTests(self):
        for test in self.tests:
            self.__testClientTransfer(test)
        return self.tests_failed

    def __removeOutput(self, filename):
        (subprocess.Popen(["rm storage/*"], shell=True)).wait()
        if self.__test_flag != "-u":
            (subprocess.Popen(["rm {}".format(filename)], shell=True)).wait()
        
    def __runDiff(self, filename):
        output_file = "storage/{}".format(filename) if self.__test_flag == "-u" else filename
        cmd = ["diff {}/{} {}".format(self.__baseline_dir, filename, output_file)]
        difftask = subprocess.Popen(cmd, stdout=subprocess.PIPE, shell=True)
        difftask.communicate()
        test_type = "Upload" if self.__test_flag == "-u" else "Download" if self.__test_flag == "-d" else "GetFiles"
        if difftask.returncode == 0:
            print("{} test success!\n".format(test_type))
            self.__removeOutput(filename) #we want to keep the faulty output for debugging
        else:
            print("{} test failed!\n".format(test_type))
            self.tests_failed += 1
        
    def __prepareForGetFiles(self, filename):
        name_list = self.__baseline_dir + "/" + filename
        with open(name_list) as names:
            names_arr = [x.strip() for x in names.readlines()]
        for name in names_arr:
            open("storage/{}".format(name), "w")

    def __grabGetFilesOutput(self, filename):
        with open("clientlog.txt") as output:
            getfiles_output = [x.strip() for x in output.readlines()]
            getfiles_output = getfiles_output[
                getfiles_output.index("[FILES]") + 1
                :
                getfiles_output.index("Client: successfully retrieved file list from server")
            ]
        with open(filename, "w") as output_file:
            for line in getfiles_output:
                output_file.write(line + "\n")

    def __prepareForDownload(self, filename):
        cmd = "cp {}/{} storage/{}".format(self.__baseline_dir, filename, filename)
        cp_task = subprocess.Popen(cmd, shell=True)
        cp_task.communicate()
        return cp_task.returncode

    def __testClientTransfer(self, filename):
        cmd = ["./client", "localhost", self.__test_flag]
        if self.__test_flag != "-g":
            cmd.append((self.__baseline_dir + "/" + filename) if self.__test_flag == "-u" else filename)
        if self.__test_flag == "-d":
            if self.__prepareForDownload(filename):
                print("Error encountered whilst preparing copy for download on file {}".format(filename))
        if self.__test_flag == "-g":
            self.__prepareForGetFiles(filename)
        with open('clientlog.txt', "w") as outfile:
            (subprocess.Popen(cmd, stdout=outfile)).wait()
        print("Ran test on: {}".format(filename))
        if self.__test_flag == "-g":
            self.__grabGetFilesOutput(filename)
        sleep(2) #wait for server to receive packets and write to file if uploading from client (problem with large files)
        self.__runDiff(filename)
        
    tests = []
    tests_failed = 0
    __test_flag = ""
    __baseline_dir = ""

def runMake():
    cmd_client = ["make", "client", "_OUTLOC=../tests/integration/"]
    cmd_server = ["make", "server", "_OUTLOC=../tests/integration/"]
    cmds = [cmd_client, cmd_server]
    for cmd in cmds:
        (subprocess.Popen(cmd, cwd="../../src", stdout=subprocess.DEVNULL)).wait()

def startServer():
    print("\n")
    with open('serverlog.txt', "w") as outfile:
        server = subprocess.Popen("./server", stdout=outfile)
        sleep(1)
        return server

def delPrograms():
   subprocess.call(["rm", "server"])
   subprocess.call(["rm", "client"])

def addTransferTests(transfer_tester, dirname):
    for file in readFiles(dirname):
        transfer_tester.addTest(file)

def performTransferTests(flag, baseline_dir):
    transfer_tester = FileTransferTester(flag, baseline_dir)
    addTransferTests(transfer_tester, baseline_dir)
    transfer_tester.runTests()
    return (len(transfer_tester.tests), transfer_tester.tests_failed)

def readFiles(dirname):
    filenames = next(walk(dirname), (None, None, []))[2]
    return filenames

def addTuples(tuple_lhs, tuple_rhs):
    return tuple(map(operator.add, tuple_lhs, tuple_rhs))

def performIntegrationTests():
    test_info = (0, 0)
    test_info = addTuples(test_info, performTransferTests("-g", "baselines/getfiles"))
    test_info = addTuples(test_info, performTransferTests("-u", "baselines/transfer"))
    test_info = addTuples(test_info, performTransferTests("-d", "baselines/transfer"))
    print(
        "[TESTS {}]\n".format("SUCCESSFUL" if test_info[1] == 0 else "FAILED"),
        "Total tests: {}\n".format(test_info[0]),
        "Successful tests: {}\n".format(test_info[0] - test_info[1]),
        "Failed tests: {}\n".format(test_info[1])
    )

def main():
    runMake()
    server = startServer()
    try:
        performIntegrationTests()
    except Exception as e:
        print(e)
    server.terminate()
    delPrograms()

if __name__ == "__main__":
    main()
