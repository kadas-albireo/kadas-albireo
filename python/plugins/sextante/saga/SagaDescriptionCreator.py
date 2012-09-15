import subprocess
import os

class SagaDescriptionCreator:


    def createLibraryFiles(self):
        f = open("c:\\saga\\sagalibs.txt")
        for lib in f:
            lib = lib.strip("\n")
            command = ["c:\\saga\\saga_cmd.exe", lib]
            f2 = open("c:\\saga\\desc\\" + lib + ".sagalib", "w")
            subprocess.Popen(command, shell=True, stdout=f2, stdin=subprocess.PIPE,stderr=subprocess.STDOUT, universal_newlines=True)
            f2.close()
        f.close()

    def createLibraryMap(self):
        self.map = {}
        for libFile in os.listdir("c:\\saga\\desc"):
            if libFile.endswith("sagalib"):
                print libFile
                algs = []
                f = open(os.path.join("c:\\saga\\desc", libFile))
                for line in f:
                    line = line.strip("\n").strip(" ")
                    digit = line.split("\t")[0]
                    print digit
                    if digit.isdigit():
                        algs.append(digit)
                self.map[libFile[:-8]] = algs
                f.close()

        print str(self.map)

    def createDescriptionFiles(self):
        for lib in self.map.keys():
            algs = self.map[lib]
            for alg in algs:
                command = ["c:\\saga\\saga_cmd.exe", lib, alg]
                f = open("c:\\saga\\desc\\" + lib + "_" + alg + ".txt", "w")
                print (str(command))
                subprocess.Popen(command, shell=True, stdout=f, stdin=subprocess.PIPE,stderr=f, universal_newlines=True)
                f.close()

    def create(self):
        self.createLibraryMap()
        self.createDescriptionFiles()

def main():

    SagaDescriptionCreator().create()


if __name__ == '__main__':
    main()
