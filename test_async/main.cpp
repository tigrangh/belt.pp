#include <iostream>
#include <chrono>
#include <exception>
#include <assert.h>
#include <async/multithreading.h>

using namespace std;

void tail_recursion(int start,
                    int end,
                    async::thread& th)
{
    //cout << start << endl;
    //this_thread::sleep_for(chrono::milliseconds(1000));

    if (start != end)
    {
        ++start;
        th.run([start, end, &th]
        {
            tail_recursion(start, end, th);
        });
    }
}

void copy()
{
    async::processor host(2);
    async::thread_pool pool(host);
    async::thread reader(pool);
    async::thread writer(pool);

    int ir = 1, iw = 0;

    auto read_task = [&ir]
    {
        if (10 == ir)
        {
            ir = 0;
            return;
        }
        cout << "begin reading " << ir << endl;
        this_thread::sleep_for(chrono::milliseconds(1000));
        cout << "end reading " << ir << endl;
    };
    auto write_task = [&iw]
    {
        if (0 == iw)
            return;
        cout << "\tbegin writing " << iw << endl;
        this_thread::sleep_for(chrono::milliseconds(10000));
        cout << "\tend writing " << iw << endl;
    };

    while (true)
    {
        writer.run(write_task);
        reader.run(read_task);

        pool.wait(async::thread_pool::all);

        if (ir == 0)
            break;

        iw = ir;
        ++ir;
    }
}

void server_messageexecutor(async::thread& logger,
                            vector<int>& messages_incoming,
                            vector<int>& messages_outgoing,
                            int message,
                            int message_index)
{
    if (message == 1)
    {
        messages_incoming.push_back(0);
        messages_outgoing.push_back(0);

        logger.run([message_index]
        {
            cout << "new message box from " << message_index << endl;
        });
    }
    else
    {
        messages_outgoing[message_index] = message + 1;

        logger.run([message_index,
                   message]
        {
            cout << "responding to " << message_index << " - " << message + 1;
        });
    }
}

void server_messageboxoperator(async::thread& messageboxoperator,
                               async::thread& messageexecutor,
                               async::thread& logger,
                               vector<int>& messages_incoming,
                               vector<int>& messages_outgoing,
                               int& message_index)
{
    if (messages_incoming[message_index])
    {
        int message = messages_incoming[message_index];
        messageexecutor.run([&logger,
                            &messages_incoming,
                            &messages_outgoing,
                            message,
                            message_index]
        {
            server_messageexecutor(logger,
                                   messages_incoming,
                                   messages_outgoing,
                                   message,
                                   message_index);
        });

        messages_incoming[message_index] = 0;
    }
    if (messages_outgoing[message_index])
    {
        messages_outgoing[message_index] = 0;
    }

    ++message_index;
    if (message_index == messages_incoming.size())
        message_index = 0;

    messageboxoperator.run([&messageboxoperator,
                           &messageexecutor,
                           &logger,
                           &messages_incoming,
                           &messages_outgoing,
                           &message_index]
    {
        server_messageboxoperator(messageboxoperator,
                                  messageexecutor,
                                  logger,
                                  messages_incoming,
                                  messages_outgoing,
                                  message_index);
    });
}

void server_simulator(async::thread& simulator,
                      vector<int>& messages_incoming)
{
    int message, index;
    cin >> index >> message;

    messages_incoming[index] = message;

    simulator.run([&simulator, &messages_incoming]
    {
        server_simulator(simulator, messages_incoming);
    });
}

void server()
{
    async::processor host(2);
    async::thread_pool pool(host);
    async::thread messageboxoperator(pool);
    async::thread messageexecutor(pool);
    async::thread logger(pool);
    async::thread simulator(pool);

    vector<int> messages_incoming;
    vector<int> messages_outgoing;
    messages_incoming.push_back(0);
    messages_outgoing.push_back(0);
    int message_index(0);

    messageboxoperator.run([&messageboxoperator,
                           &messageexecutor,
                           &logger,
                           &messages_incoming,
                           &messages_outgoing,
                           &message_index]
    {
        server_messageboxoperator(messageboxoperator,
                                  messageexecutor,
                                  logger,
                                  messages_incoming,
                                  messages_outgoing,
                                  message_index);
    });

    simulator.run([&simulator,
                  &messages_incoming]
    {
        server_simulator(simulator, messages_incoming);
    });

    pool.wait(async::thread_pool::all);
}

void terminal_user_interaction(async::thread& th, bool& bStop)
{
    int a;
    cin >> a;

    if (0 == a)
    {
        bStop = true;
    }
    else
    {
        th.run([&th, &bStop]
        {
            terminal_user_interaction(th, bStop);
        });
    }
}

void terminal_worker(int& i, async::thread& th, bool& bStop)
{
    ++i;
    cout << i << endl;

    if (false == bStop)
    {
        th.run([&i, &th, &bStop]
        {
            terminal_worker(i, th, bStop);
        });
    }
}

void terminal()
{
    async::processor host(1);
    async::thread_pool pool(host);
    async::thread user_interaction(pool);
    async::thread worker(pool);

    bool bStop = false;

    int i = 0;

    user_interaction.run([&user_interaction, &bStop]
    {
        terminal_user_interaction(user_interaction, bStop);
    });

    worker.run([&worker, &bStop, &i]
    {
        terminal_worker(i, worker, bStop);
    });
}

int main()
{
    server();
    //terminal();
    //copy();
    return 0;
    try
    {
        auto tp_start = chrono::steady_clock::now();
        auto tp_end = tp_start;

        async::processor host(1);
        async::thread_pool pool(host);
        async::thread th1(pool);
        async::thread th2(pool);

#define COUNT 10000000
        async::processor::task taskob1 = [&th1]
        {
            tail_recursion(1, 1, th1);
        };
        async::processor::task taskob2 = [&th2]
        {
            tail_recursion(101, 101, th2);
        };

        th1.run(taskob1);
        th2.run(taskob2);

        //host.setCoreCount(5);
        //host.setCoreCount(2);

        async::thread_pool::threadset threads_done;
        threads_done = pool.wait(async::thread_pool::any);
        if (threads_done.find(&th1) != threads_done.end() &&
            threads_done.find(&th2) != threads_done.end())
            cout << "1&2 done\n";
        else if (threads_done.find(&th1) != threads_done.end())
            cout << "1 done\n";
        else if (threads_done.find(&th2) != threads_done.end())
            cout << "2 done\n";
        threads_done = pool.wait(async::thread_pool::any);
        if (threads_done.find(&th1) != threads_done.end() &&
            threads_done.find(&th2) != threads_done.end())
            cout << "1&2 done\n";
        else if (threads_done.find(&th1) != threads_done.end())
            cout << "1 done\n";
        else if (threads_done.find(&th2) != threads_done.end())
            cout << "2 done\n";

        try
        {
            th1.wait();
        }
        catch(int value)
        {
            cout << value << " thrown\n";
        }
        try
        {
            th2.wait();
        }
        catch(int value)
        {
            cout << value << " thrown\n";
        }

        //threads_done = pool.wait();
        //assert(threads_done.empty());
        //threads_done = pool.wait();

        //host.setCoreCount(2);
        //th1.wait();
        //th2.wait();

        for (int i = 0; i < 1100; ++i)
        {
            //th1.run([]{ this_thread::sleep_for(chrono::microseconds(1));});
            //th2.run([]{ this_thread::sleep_for(chrono::microseconds(1));});
            th1.run([i]
            {
                if (i == 100)
                    throw 100;
                cout << i << endl;
            });
            th1.run([i]
            {
                if (i == 101)
                    throw 101;
                cout << i << endl;
            });
        }
        //th1.wait();
        //th2.wait();
        pool.wait(async::thread_pool::any);
        pool.wait(async::thread_pool::any);
        pool.wait(async::thread_pool::all);
        pool.wait(async::thread_pool::all);

        tp_end = chrono::steady_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(tp_end - tp_start);
        cout << "duration " << duration.count() << " millisecond" << endl;
    }
    catch (...)
    {
        cout << "unknown exception\n";
    }

    cout << "done\n";

    return 0;
}
