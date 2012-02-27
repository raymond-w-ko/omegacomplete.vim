import socket
import sys
import string
import threading
import os
import re
from threading import Thread, RLock
from omegacomplete.utils import safe_recv, safe_sendall, normalize_path_separators,log

class KeywordAnalyzer(Thread):
    def __init__(self, parent):
        Thread.__init__(self)
        self.parent = parent

    def calculate_abbrev(self, word):
        underscore = []
        camelcase = []

        underscore.append(word[0])
        camelcase.append(word[0])

        append_next_to_underscore = False
        for ii in xrange(1, len(word), 1):
            c = word[ii]

            if append_next_to_underscore and c != "_":
                underscore.append(c)
                append_next_to_underscore = False

            if c == "_":
                append_next_to_underscore = True

            if c.isupper():
                camelcase.append(c)

        if len(underscore) > 1:
            underscore = "".join(underscore)
            underscores = self.parent.underscores

            if underscore != word:
                if underscore not in underscores:
                    underscores[underscore] = set()

                underscores[underscore].add(word)

        if len(camelcase) > 1:
            camelcase = "".join(camelcase)
            camelcases = self.parent.camelcases
            if camelcase != word:
                if camelcase not in camelcases:
                    camelcases[camelcase] = set()

                camelcases[camelcase].add(word)

    def run(self):
        tokens = re.split(r"\W+", self.parent.contents)

        self.parent.rlock.acquire()

        self.parent.word_set.clear()

        for word in tokens:
            if len(word) == 0:
                continue

            self.parent.word_set.add(word)
            self.calculate_abbrev(word)

        for abbrev, words in self.parent.camelcases.items():
            log(abbrev + ": " + str(words))

        for abbrev, words in self.parent.underscores.items():
            log(abbrev + ": " + str(words))

        self.parent.rlock.release()


class Buffer:
    def __init__(self, filename):
        self.filename = filename
        self.last_modification_time = 0
        self.keyword_analyzer = None
        self.contents = ""

        self.rlock = RLock()

        self.word_set = set()
        self.underscores = {}
        self.camelcases = {}

    def clear():
        self.rlock.acquire()

        self.word_set.clear()
        self.underscores.clear()
        self.camelcases.clear()

        self.rlock.release()

    def parse(self):
        try:
            statinfo = os.stat(self.filename)
        except:
            log("unable to stat: " + self.filename)
            return False

        if statinfo.st_mtime <= self.last_modification_time:
            log("no processing necessary, already up-to-date")
            return True

        self.last_modification_time = statinfo.st_mtime

        try:
            temp_contents = open(self.filename).read()
        except:
            log("unable to read contents of file: " + self.filename)
            return False

        self.rlock.acquire()
        self.contents = temp_contents
        self.rlock.release()

        self.keyword_analyzer = KeywordAnalyzer(self)
        self.keyword_analyzer.start()

        return True

    def parse_with_contents(self, new_contents):

        return True


class OmegaCompleteServer:
    def __init__(self):
        self.buffers = {}
        self.active_buffer = None

    def handle(self, cmd, arg):
        if cmd == "file_open":
            log("file_open")

            filename = normalize_path_separators(arg)
            log("considering normalized filename: " + filename)

            if filename in self.buffers:
                log("already created buffer object, updating...")
                if not self.buffers[filename].parse():
                    log("failed to re-parse() buffer")
            else:
                log("created new buffer object for: " + filename)
                buffer = Buffer(filename)
                self.buffers[filename] = buffer
                if not buffer.parse():
                    log("failed to do initial processing of buffer")

            return ""
        elif cmd == "current_file":
            filename = normalize_path_separators(arg)
            self.active_buffer = self.buffers[filename]

            log("setting active buffer object to: " + filename)

            assert self.active_buffer != None

            return ""
        elif cmd == "buffer_contents":
            if not self.active_buffer.parse_with_contents(arg):
                log("failed to update buffer_contents")

            return ""

        return "unhandled"


def main(*args):
    HOST = "localhost"
    PORT = 31337
    ocs = OmegaCompleteServer()

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind((HOST, PORT))
    s.listen(5)
    while True:
        (conn, address) = s.accept()
        msg = safe_recv(conn)
        if len(msg) == 0:
            continue

        msg = string.split(msg, None, 1)

        if (msg[0] == "exit" or msg[0] == "quit"):
            safe_sendall(conn, "exiting")
            conn.close()
            break

        if (len(msg) != 2):
            safe_sendall(conn, "invalid command")
            conn.close()
            break

        safe_sendall(conn, ocs.handle(msg[0], msg[1]))
        conn.close()

    return 0

if __name__ == '__main__':
    sys.exit(main(*sys.argv))
