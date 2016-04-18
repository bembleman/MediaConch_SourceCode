/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a GPLv3+/MPLv2+ license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// NoDatabase functions
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef NODATABASEUIH
#define NODATABASEUIH

//---------------------------------------------------------------------------
#include "DatabaseUi.h"

//---------------------------------------------------------------------------
namespace MediaConch {

class FileRegistered;

//***************************************************************************
// Class NoDatabase
//***************************************************************************

class NoDatabaseUi : public DatabaseUi
{
public:
    //Constructor/Destructor
    NoDatabaseUi();
    virtual ~NoDatabaseUi();

    virtual int init();
    virtual int init_ui();

    // UI
    virtual int ui_add_file(const FileRegistered* file);
    virtual int ui_add_files(const std::vector<FileRegistered*>& files);
    virtual int ui_update_file(const FileRegistered* file);
    virtual int ui_update_files(const std::vector<FileRegistered*>& files);
    virtual int ui_get_file(FileRegistered* file);
    virtual int ui_remove_file(const FileRegistered* file);
    virtual int ui_remove_files(const std::vector<FileRegistered*>& files);
    virtual int ui_remove_all_files();
    virtual void ui_get_elements(std::vector<FileRegistered*>& vec);

protected:
    virtual int execute();

private:
    NoDatabaseUi (const NoDatabaseUi&);
    NoDatabaseUi& operator=(const NoDatabaseUi&);
};

}

#endif /* !NODATABASEUIH */
