/*
 * Copyright 2011, 2012 Esrille Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// A test harness for the implementation report of
//       the CSS2.1 Conformance Test Suite
// http://test.csswg.org/suites/css2.1/20110323/

#include <unistd.h>
#include <sys/wait.h>

#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <boost/version.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>

enum {
    INTERACTIVE = 0,
    HEADLESS,
    REPORT,
    UPDATE,
    GENERATE,

    // exit status codes
    ES_PASS = 0,
    ES_FATAL,
    ES_FAIL,
    ES_INVALID,
    ES_NA,
    ES_SKIP,
    ES_UNCERTAIN
};

const char* StatusStrings[] = {
    "pass",
    "fatal",
    "fail",
    "invalid",
    "?",
    "skip",
    "uncertain",
};

struct ForkStatus {
    pid_t       pid;
    std::string url;
    int         code;
public:
    ForkStatus() : pid(-1), code(-1) {}
};

size_t forkMax = 1;
size_t forkTop = 0;
size_t forkCount = 0;
std::vector<ForkStatus> forkStates(1);

size_t uncertainCount = 0;

int processOutput(std::istream& stream, std::string& result)
{
    std::string output;
    bool completed = false;
    while (std::getline(stream, output)) {
        if (!completed) {
            if (output == "## complete")
                completed = true;
            continue;
        }
        if (output == "##")
            break;
        result += output + '\n';
    }
    return 0;
}

int runTest(int argc, char* argv[], std::string userStyle, std::string testFonts, std::string url, std::string& result, unsigned timeout)
{
    int pipefd[2];
    pipe(pipefd);
    char testfontsOption[] = "-testfonts";

    pid_t pid = fork();
    if (pid == -1) {
        std::cerr << "error: no more process to create\n";
        return -1;
    }
    if (pid == 0) {
        close(1);
        dup(pipefd[1]);
        close(pipefd[0]);
        int argi = argc - 1;
        if (!userStyle.empty())
            argv[argi++] = strdup(userStyle.c_str());
        if (testFonts == "on")
            argv[argi++] = testfontsOption;
        url = "http://localhost/suites/css2.1/20110323/" + url;
        // url = "http://test.csswg.org/suites/css2.1/20110323/" + url;
        argv[argi++] = strdup(url.c_str());
        argv[argi] = 0;
        if (timeout)
            alarm(timeout); // Terminate the process if it does not complete in 'timeout' seconds.
        execvp(argv[0], argv);
        exit(EXIT_FAILURE);
    }
    close(pipefd[1]);

#if 104400 <= BOOST_VERSION
    boost::iostreams::stream<boost::iostreams::file_descriptor_source> stream(pipefd[0], boost::iostreams::close_handle);
#else
    boost::iostreams::stream<boost::iostreams::file_descriptor_source> stream(pipefd[0], true);
#endif
    processOutput(stream, result);
    return pid;
}

void killTest(int pid)
{
    int status;
    kill(pid, SIGTERM);
    if (wait(&status) == -1)
        std::cerr << "error: failed to wait for a test process to complete\n";
}

bool loadLog(const std::string& path, std::string& result, std::string& log)
{
    std::ifstream file(path.c_str());
    if (!file) {
        result = "?";
        return false;
    }
    std::string line;
    std::getline(file, line);
    size_t pos = line.find('\t');
    if (pos != std::string::npos)
        result = line.substr(pos + 1);
    else {
        result = "?";
        return false;
    }
    log.clear();
    while (std::getline(file, line))
        log += line + '\n';
    return true;
}

bool saveLog(const std::string& path, const std::string& url, const std::string& result, const std::string& log)
{
    std::ofstream file(path.c_str(), std::ios_base::out | std::ios_base::trunc);
    if (!file) {
        std::cerr << "error: failed to open the report file\n";
        return false;
    }
    file << "# " << url.c_str() << '\t' << result << '\n' << log;
    file.flush();
    file.close();
    return true;
}

std::string test(int mode, int argc, char* argv[], const std::string& url, const std::string& userStyle, const std::string& testFonts, unsigned timeout)
{
    std::string path(url);
    size_t pos = path.rfind('.');
    if (pos != std::string::npos) {
        path.erase(pos);
        path += ".log";
    }
    std::string evaluation;
    std::string log;
    loadLog(path, evaluation, log);

    pid_t pid = -1;
    std::string output;
    switch (mode) {
    case REPORT:
        break;
    case UPDATE:
        if (evaluation[0] == '?')
            break;
        // FALL THROUGH
    default:
        pid = runTest(argc, argv, userStyle, testFonts, url, output, timeout);
        break;
    }

    std::string result;
    if (0 < pid && output.empty())
        result = "fatal";
    else if (mode == INTERACTIVE) {
        std::cout << "## complete\n" << output;
        std::cout << '[' << url << "] ";
        if (evaluation.empty() || evaluation[0] == '?')
            std::cout << "pass? ";
        else {
            std::cout << evaluation << "? ";
            if (evaluation != "pass")
                std::cout << '\a';
        }
        std::getline(std::cin, result);
        if (result.empty()) {
            if (evaluation.empty() || evaluation[0] == '?')
                result = "pass";
            else
                result = evaluation;
        } else if (result == "p" || result == "\x1b")
            result = "pass";
        else if (result == "f")
            result = "fail";
        else if (result == "i")
            result = "invalid";
        else if (result == "k") // keep
            result = evaluation;
        else if (result == "n")
            result = "na";
        else if (result == "s")
            result = "skip";
        else if (result == "u")
            result = "uncertain";
        else if (result == "q" || result == "quit")
            exit(EXIT_FAILURE);
        else if (result == "z")
            result = "undo";
        if (result != "undo" && !saveLog(path, url, result, output)) {
            std::cerr << "error: failed to open the report file\n";
            exit(EXIT_FAILURE);
        }
    } else if (mode == HEADLESS) {
        if (evaluation != "?" && output != log)
            result = "uncertain";
        else
            result = evaluation;
    } else if (mode == REPORT) {
        result = evaluation;
    } else if (mode == UPDATE) {
        result = evaluation;
        if (result[0] != '?') {
            if (!saveLog(path, url, result, output)) {
                std::cerr << "error: failed to open the report file\n";
                exit(EXIT_FAILURE);
            }
        }
    }

    if (0 < pid)
        killTest(pid);
    if (mode != INTERACTIVE && result[0] != '?')
        std::cout << url << '\t' << result << '\n';
    return result;
}

int reduce(std::ostream& report, int option = 0)
{
    size_t count;
    int op = (forkCount < forkMax) ? option : 0;
    for (count = 0; count < forkCount; ++count) {
        int status;
        pid_t pid = waitpid(-1, &status, op);
        if (pid == 0)
            break;
        for (size_t i = 0; i < forkCount; ++i) {
            auto s = &forkStates[(forkTop + i) % forkMax];
            if (s->pid == pid) {
                s->code = WIFEXITED(status) ? WEXITSTATUS(status) : ES_FATAL;
                if (i == 0)
                    op = option;
                break;
            }
        }
    }
    if (0 < count) {
        for (count = 0; count < forkMax; ++count) {
            auto s = &forkStates[(forkTop + count) % forkMax];
            if (s->code == -1)
                break;
            if (s->code == ES_UNCERTAIN)
                ++uncertainCount;
            if (s->code != ES_NA)
                std::cout << s->url << '\t' << StatusStrings[s->code] << '\n';
            report << s->url << '\t' << StatusStrings[s->code] << '\n';
            s->code = -1;
        }
        assert(count <= forkCount);
        forkTop += count;
        forkTop %= forkMax;
        forkCount -= count;
    }
    return count;
}

void map(std::ostream& report, int mode, int argc, char* argv[], const std::string& url, const std::string& userStyle, const std::string& testFonts, unsigned timeout, std::string result = "")
{
    assert(forkCount < forkMax);
    pid_t pid = fork();
    if (pid == -1) {
        std::cerr << "error: no more process to create\n";
        exit(EXIT_FAILURE);
    }
    if (pid == 0) {
        std::string path(url);
        size_t pos = path.rfind('.');
        if (pos != std::string::npos) {
            path.erase(pos);
            path += ".log";
        }
        std::string evaluation;
        std::string log;
        loadLog(path, evaluation, log);

        pid_t pid = -1;
        std::string output;
        switch (mode) {
        case GENERATE:
            evaluation = result;
            // FALL THROUGH
        case UPDATE:
            if (evaluation[0] == '?')
                break;
            // FALL THROUGH
        default:
            pid = runTest(argc, argv, userStyle, testFonts, url, output, timeout);
            break;
        }

        if (0 < pid && output.empty())
            result = "fatal";
        else {
            switch (mode) {
            case HEADLESS:
                if (evaluation != "?" && output != log)
                    result = "uncertain";
                else
                    result = evaluation;
                break;
            case UPDATE:
            case GENERATE:
                result = evaluation;
                if (result[0] != '?') {
                    if (!saveLog(path, url, result, output)) {
                        std::cerr << "error: failed to open the report file\n";
                        exit(EXIT_FAILURE);
                    }
                }
                break;
            default:
                break;
            }
        }
        if (0 < pid)
            killTest(pid);
        int status = ES_NA;
        if (!result.compare(0, 4, "pass"))
            status = ES_PASS;
        else if (!result.compare(0, 5, "fatal"))
            status = ES_FATAL;
        else if (!result.compare(0, 4, "fail"))
            status = ES_FAIL;
        else if (!result.compare(0, 7, "invalid"))
            status = ES_INVALID;
        else if (!result.compare(0, 4, "skip"))
            status = ES_SKIP;
        else if (!result.compare(0, 9, "uncertain"))
            status = ES_UNCERTAIN;
        exit(status);
    } else {
        auto s = &forkStates[(forkTop + forkCount) % forkMax];
        s->url = url;
        s->pid = pid;
        ++forkCount;
        reduce(report, WNOHANG);
    }
}

int main(int argc, char* argv[])
{
    int mode = HEADLESS;
    unsigned timeout = 10;

    int argi = 1;
    while (*argv[argi] == '-') {
        switch (argv[argi][1]) {
        case 'i':
            mode = INTERACTIVE;
            timeout = 0;
            break;
        case 'g':
            mode = GENERATE;
            break;
        case 'r':
            mode = REPORT;
            break;
        case 'u':
            mode = UPDATE;
            break;
        case 'j':
            forkMax = strtoul(argv[argi] + 2, 0, 10);
            if (forkMax == 0)
                forkMax = 1;
            forkStates.resize(forkMax);
            break;
        default:
            break;
        }
        ++argi;
    }

    if (argc < argi + 2) {
        std::cout << "usage: " << argv[0] << " [-i] report.data command [argument ...]\n";
        return EXIT_FAILURE;
    }

    std::ifstream data(argv[argi]);
    if (!data) {
        std::cerr << "error: " << argv[argi] << ": no such file\n";
        return EXIT_FAILURE;
    }

    std::ofstream report("report.data", std::ios_base::out | std::ios_base::trunc);
    if (!report) {
        std::cerr << "error: failed to open the report file\n";
        return EXIT_FAILURE;
    }

    char* args[argc - argi + 3];
    for (int i = 2; i < argc; ++i)
        args[i - 2] = argv[i + argi - 1];
    args[argc - argi] = args[argc - argi + 1] = args[argc - argi + 2] = 0;

    std::string result;
    std::string url;
    std::string undo;
    std::string userStyle;
    std::string testFonts;
    bool redo = false;
    while (data) {
        if (result == "undo") {
            std::swap(url, undo);
            redo = true;
        } else if (redo) {
            std::swap(url, undo);
            redo = false;
        } else {
            std::string line;
            std::getline(data, line);
            if (line.empty())
                continue;
            if (line == "testname    result  comment") {
                report << line << '\n';
                continue;
            }
            if (line[0] == '#') {
                if (line.compare(1, 9, "userstyle") == 0) {
                    if (10 < line.length()) {
                        std::stringstream s(line.substr(10), std::stringstream::in);
                        s >> userStyle;
                    } else
                        userStyle.clear();
                } else if (line.compare(1, 9, "testfonts") == 0) {
                    if (10 < line.length()) {
                        std::stringstream s(line.substr(10), std::stringstream::in);
                        s >> testFonts;
                    } else
                        testFonts.clear();
                }
                reduce(report);
                report << line << '\n';
                continue;
            }
            undo = url;
            std::stringstream s(line, std::stringstream::in);
            s >> url;
            s >> result;
            std::getline(s, line);
            result += line;
        }
        if (url.empty())
            continue;
        if (result.empty())
            result = "?";

        switch (mode) {
        case HEADLESS:
        case UPDATE:
        case GENERATE:
            map(report, mode, argc - argi, args, url, userStyle, testFonts, timeout, result);
            break;
        default:
            result = test(mode, argc - argi, args, url, userStyle, testFonts, timeout);
            if (result != "undo")
                report << url << '\t' << result << '\n';
            break;
        }
    }
    if (mode == HEADLESS || mode == UPDATE)
        reduce(report);
    report.close();

    return (0 < uncertainCount) ? EXIT_FAILURE : EXIT_SUCCESS;
}
