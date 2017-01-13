/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a GPLv3+/MPLv2+ license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifdef __BORLANDC__
    #pragma hdrstop
#endif
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include <ZenLib/File.h>

#include "Queue.h"
#include "Scheduler.h"
#include "PluginLog.h"
#include "Core.h"
#include <fstream>

#if !defined(WINDOWS)
#include <unistd.h>
#endif //!defined(WINDOWS)

#include <algorithm>

//---------------------------------------------------------------------------
namespace MediaConch {

//---------------------------------------------------------------------------
QueueElement::QueueElement(Scheduler *s) : Thread(), scheduler(s), MI(NULL)
{
}

//---------------------------------------------------------------------------
QueueElement::~QueueElement()
{
}

//---------------------------------------------------------------------------
void QueueElement::stop()
{
    RequestTerminate();
    while (!IsExited())
    {
#ifdef WINDOWS
        Sleep(0);
#else //WINDOWS
        sleep(0);
#endif //WINDOWS
    }
}


static void __stdcall Event_CallBackFunction(unsigned char* Data_Content, size_t, void* UserHandle_Void)
{
    QueueElement *queue = (QueueElement*)UserHandle_Void;
    struct MediaInfo_Event_Generic* Event_Generic = (struct MediaInfo_Event_Generic*)Data_Content;
    // unsigned char ParserID = (unsigned char)((Event_Generic->EventCode & 0xFF000000) >> 24);
    unsigned short EventID = (unsigned short)((Event_Generic->EventCode & 0x00FFFF00) >> 8);
    unsigned char EventVersion = (unsigned char)(Event_Generic->EventCode & 0x000000FF);

    switch (EventID)
    {
        case MediaInfo_Event_Global_AttachedFile:
            if (EventVersion == 0)
                queue->attachment_cb((struct MediaInfo_Event_Global_AttachedFile_0 *)Data_Content);
            break;
        default:
            break;
    }
}

//---------------------------------------------------------------------------
void QueueElement::Entry()
{
    std::string file = real_filename;
    std::string err;

    //Pre hook plugins
    int ret = 0;

    std::stringstream log;
    log << "start analyze:" << file;
    scheduler->write_log_timestamp(PluginLog::LOG_LEVEL_DEBUG, log.str());

    ret = scheduler->execute_pre_hook_plugins(this, err);

    if (ret || !mil_analyze)
    {
        log.str("");
        log << "end analyze:" << file;
        scheduler->write_log_timestamp(PluginLog::LOG_LEVEL_DEBUG, log.str());
        scheduler->work_finished(this, NULL);
        return;
    }

    MI = new MediaInfoNameSpace::MediaInfo;

    // Currently avoiding to have a big trace
    bool found = false;
    for (size_t i = 0; i < options.size(); ++i)
        if (options[i].first == "parsespeed")
            found = true;
    if (found == false)
        MI->Option(__T("ParseSpeed"), __T("0"));

    // Configuration of the parsing
    found = false;
    for (size_t i = 0; i < options.size(); ++i)
        if (options[i].first == "details")
            found = true;
    if (found == false)
        MI->Option(__T("Details"), __T("1"));

    // Attachment
    std::stringstream ss;
    ss << "CallBack=memory://" << (int64u)Event_CallBackFunction << ";UserHandler=memory://" << (int64u)this;
    MI->Option(__T("File_Event_CallBackFunction"), ZenLib::Ztring().From_UTF8(ss.str()));

    // Partial configuration of the output (note: this options should be removed after libmediainfo has a support of these options after Open() )
    MI->Option(__T("ReadByHuman"), __T("1"));
    MI->Option(__T("Language"), __T("raw"));
    MI->Option(__T("Inform"), __T("MICRO_XML"));

    for (size_t i = 0; i < options.size(); ++i)
        MI->Option(Ztring().From_UTF8(options[i].first), Ztring().From_UTF8(options[i].second));

    MI->Open(ZenLib::Ztring().From_UTF8(file));
    scheduler->work_finished(this, MI);
    MI->Close();
    delete MI;
    MI = NULL;
    log.str("");
    log << "end analyze:" << file;
    scheduler->write_log_timestamp(PluginLog::LOG_LEVEL_DEBUG, log.str());

    // Delete a generated file
    if (real_filename != filename)
    {
        Ztring z_path = ZenLib::Ztring().From_UTF8(real_filename);
        ZenLib::File::Delete(z_path);
    }
}

//---------------------------------------------------------------------------
double QueueElement::percent_done()
{
    if (!MI)
        return (double)0;

    size_t state = MI->State_Get();
    return (double)state / 100;
}

//---------------------------------------------------------------------------
int QueueElement::attachment_cb(struct MediaInfo_Event_Global_AttachedFile_0 *Event)
{
    std::string attachment((const char*)Event->Content, Event->Content_Size);

    std::string realname = "Unknown";
    if (Event->Name && Event->Name[0])
        realname = std::string(Event->Name);

    std::string path;

    if (Core::create_local_unique_data_filename("MediaconchTemp", "attachment", "", path) < 0)
        return 0;

    std::ofstream ofs(path.c_str(), std::ofstream::out);
    ofs.write(attachment.c_str(), attachment.size());
    ofs.close();

    Attachment *attach = new Attachment;
    attach->filename = path;
    attach->realname = realname;

    attachments.push_back(attach);

    return 0;
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
Queue::~Queue()
{
    clear();
}

int Queue::add_element(QueuePriority priority, int id, int user, const std::string& filename, long file_id,
                       const std::vector<std::pair<std::string,std::string> >& options,
                       const std::vector<std::string>& plugins, bool mil_analyze, const std::string& alias)
{
    QueueElement *el = new QueueElement(scheduler);

    el->id = id;
    el->user = user;
    el->filename = alias.size() ? alias : filename;
    el->real_filename = filename;
    el->file_id = file_id;
    el->mil_analyze = mil_analyze;

    std::vector<std::pair<std::string,std::string> > opts;
    for (size_t i = 0; i < options.size(); ++i)
    {
        std::string key_option = options[i].first;
        std::string value_option = options[i].second;

        if (!key_option.size())
            continue;

        transform(key_option.begin(), key_option.end(), key_option.begin(), (int(*)(int))tolower);
        // transform(value_option.begin(), value_option.end(), value_option.begin(), (int(*)(int))tolower);

        opts.push_back(std::make_pair(key_option, value_option));
    }

    el->options_str = Core::serialize_string_from_options_vec(opts);
    for (size_t i = 0; i < opts.size(); ++i)
        el->options.push_back(std::make_pair(opts[i].first, opts[i].second));

    for (size_t i = 0; i < plugins.size(); ++i)
        el->plugins.push_back(plugins[i]);

    queue[priority].push_back(el);
    return 0;
}

long Queue::has_element(int user, const std::string& filename)
{
    std::map<QueuePriority, std::list<QueueElement*> >::iterator it = queue.begin();

    for (; it != queue.end(); ++it)
    {
        std::list<QueueElement*>::iterator it_l = it->second.begin();
        for (; it_l != it->second.end() ; ++it_l)
            if ((*it_l)->filename == filename && (*it_l)->user == user)
                return (*it_l)->file_id;
    }

    return -1;
}

int Queue::has_id(int user, long file_id)
{
    std::map<QueuePriority, std::list<QueueElement*> >::iterator it = queue.begin();

    for (; it != queue.end(); ++it)
    {
        std::list<QueueElement*>::iterator it_l = it->second.begin();
        for (; it_l != it->second.end() ; ++it_l)
            if ((*it_l)->file_id == file_id && (*it_l)->user == user)
                return 0;
    }

    return -1;
}

int Queue::remove_element(int id)
{
    std::map<QueuePriority, std::list<QueueElement*> >::iterator it = queue.begin();

    for (; it != queue.end(); ++it)
    {
        std::list<QueueElement*>::iterator it_l = it->second.begin();
        for (; it_l != it->second.end() ; ++it_l)
            if ((*it_l)->id == id)
            {
                delete *it_l;
                it_l = it->second.erase(it_l);
            }
    }
    return 0;
}

int Queue::remove_elements(int user, const std::string& filename)
{
    std::map<QueuePriority, std::list<QueueElement*> >::iterator it = queue.begin();

    for (; it != queue.end(); ++it)
    {
        std::list<QueueElement*>::iterator it_l = it->second.begin();
        for (; it_l != it->second.end() ; ++it_l)
            if ((*it_l)->filename == filename && (*it_l)->user == user)
            {
                delete *it_l;
                it_l = it->second.erase(it_l);
            }
    }
    return 0;
}

void Queue::clear()
{
    std::map<QueuePriority, std::list<QueueElement*> >::iterator it = queue.begin();

    for (; it != queue.end(); ++it)
    {
        std::list<QueueElement*>::iterator it_l = it->second.begin();
        for (; it_l != it->second.end() ; ++it_l)
            delete *it_l;
    }
    queue.clear();
}

QueueElement *Queue::run_next()
{
    QueueElement* el = NULL;

    if (!queue[PRIORITY_HIGH].empty())
    {
        el = queue[PRIORITY_HIGH].front();
        queue[PRIORITY_HIGH].pop_front();
    }
    else if (!queue[PRIORITY_MEDIUM].empty())
    {
        el = queue[PRIORITY_MEDIUM].front();
        queue[PRIORITY_MEDIUM].pop_front();
    }
    else if (!queue[PRIORITY_LOW].empty())
    {
        el = queue[PRIORITY_LOW].front();
        queue[PRIORITY_LOW].pop_front();
    }
    else if (!queue[PRIORITY_NONE].empty())
    {
        el = queue[PRIORITY_NONE].front();
        queue[PRIORITY_NONE].pop_front();
    }

    if (!el)
        return NULL;

    el->Run();
    return el;
}

}
