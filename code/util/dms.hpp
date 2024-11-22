#include"log.hpp"
#include<python3.8/Python.h>
namespace MindbniM
{
    class DMSClient
    {
    public:
        using ptr=std::shared_ptr<DMSClient>;
        DMSClient()
        {
            Py_Initialize();
            PyRun_SimpleString("import sys; sys.path.append('/home/mindbnim/MicroChatRouting/code/util/')");
            _pModule=PyImport_ImportModule("dms");
            if(_pModule==nullptr)
            {
                LOG_ROOT_ERROR<<"短信模块加载失败";
                exit(0);
            }
            _pClass=PyObject_GetAttrString(_pModule,"DMSClient");
            if (!_pClass || !PyCallable_Check(_pClass)) 
            {
                LOG_ROOT_ERROR<<"短信模块实例加载失败";
                exit(0);
            }
            _pInstance = PyObject_CallObject(_pClass, NULL);
            if(_pInstance==nullptr)
            {
                LOG_ROOT_ERROR<<"短信模块实例化失败";
                exit(0);
            }
            _pSend = PyObject_GetAttrString(_pInstance, "send");
            if(_pSend==nullptr)
            {
                LOG_ROOT_ERROR<<"短信发送模块实例化失败";
                exit(0);
            }
        }
        ~DMSClient()
        {
            Py_DECREF(_pModule);
            Py_DECREF(_pClass);
            Py_DECREF(_pInstance);
            Py_DECREF(_pSend);
        }
        void Send(const std::string& phone,const std::string& code)
        {
            PyObject* pArgs = PyTuple_Pack(2, PyUnicode_FromString(phone.c_str()),PyUnicode_FromString(code.c_str()));
            PyObject* pResult = PyObject_CallObject(_pSend, pArgs);
            std::string result;
            if (pResult) 
            {
                result=PyUnicode_AsUTF8(pResult);
                Py_DECREF(pResult);
            }
            else
            {
                LOG_ROOT_ERROR<<"短信发送失败";
                Py_DECREF(pArgs);
                return;
            }
            Py_DECREF(pArgs);
            if(result!="")
            {
                LOG_ROOT_ERROR<<"短信发送失败 : "<<result;
            }
            return;
        }
    private:
        PyObject* _pModule;
        PyObject* _pClass;
        PyObject* _pInstance;
        PyObject* _pSend;
    };
}