#include "Device.h"
#include "../Model/Model.h"

void FreeWill::Device<FreeWill::DeviceType::GPU_CUDA>::pushWork(FreeWill::WorkerMessage *message)
{
    m_commandQueue.push(message);

    message->thread_id = 1;

}

void FreeWill::Device<FreeWill::DeviceType::GPU_CUDA>::terminate()
{
    FreeWill::WorkerMessage message(FreeWill::WorkerMessage::Type::TERMINATE, (FreeWill::Operator<FreeWill::DeviceType::GPU_CUDA>*)nullptr);
    pushWork(&message);
    message.join();
    m_workerThread->join();
    delete m_workerThread;
    m_workerThread = nullptr;
}

static std::mutex outputLock;

void FreeWill::Device<FreeWill::DeviceType::GPU_CUDA>::threadLoop()
{
    std::thread::id this_id = std::this_thread::get_id();

    RUN_CUDA(cudaSetDevice(m_cudaDeviceId));

    while(!m_finished)
    {
        FreeWill::WorkerMessage *message = m_commandQueue.pop();

        if (message->workType() == FreeWill::WorkerMessage::Type::TERMINATE)
        {
            message->done();
            break;
        }
        /*{
            std::unique_lock<std::mutex> ol(outputLock);
            std::cout << "thread: " << this_id << " device "<< m_deviceId << " output." << message->debug_num << std::endl;
        }*/
        {
            Operator<FreeWill::DeviceType::GPU_CUDA> *operatorBase = message->template operatorBase<FreeWill::DeviceType::GPU_CUDA>();
            operatorBase->evaluate();
        }

        message->done();

    }

    //std::cout << " terminated"<<std::endl;
}

void FreeWill::Device<FreeWill::DeviceType::GPU_CUDA>::init()
{
    m_workerThread = new std::thread([=]{FreeWill::Device<FreeWill::DeviceType::GPU_CUDA>::threadLoop();});
    /*cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(m_deviceId, &cpuset);
    int rc = pthread_setaffinity_np(m_workerThread->native_handle(),
                                    sizeof(cpu_set_t), &cpuset);*/
     //struct sched_param param = {0};
     //param.sched_priority = 99;

    //pthread_setschedparam(m_workerThread->native_handle(), SCHED_BATCH, &param);

    RUN_CUDA( cudaSetDevice(m_cudaDeviceId));
    RUN_CUDNN( cudnnCreate(&m_cudnnHandle));
    RUN_CUBLAS( cublasCreate(&m_cublasHandle));
}
