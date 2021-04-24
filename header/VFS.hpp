/*
 * @Author: your name
 * @Date: 2020-12-07 18:33:43
 * @LastEditTime: 2020-12-07 21:44:57
 * @LastEditors: Please set LastEditors
 * @Description: The VFS main header
 * @FilePath: /code/VFS.hpp
 */
#ifndef _VFS_HPP_
#define _VFS_HPP_

#define FIND_FALSE 0xffffffff

#include "./superblock.hpp"
#include "./block_bitmap.hpp"
#include "./inode_bitmap.hpp"
#include "./inode.hpp"
#include "./file_block.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

class VFS
{
protected:
    Superblock          superblock;     // superblock ��פ�ڴ�
    Block_bitmap        block_bitmap;   // block_bitmap ��פ�ڴ�
    Inode_bitmap        inode_bitmap;   // inode_bitmap ��פ�ڴ�
    std::vector<Inode>  inode_table;    // inode_table  ��פ�ڴ�
    std::string         path = "./disk_in_disk.txt";

    uint16_t             VFS_login_uid = 0;      // ��¼���ļ�ϵͳ�� uid
    uint16_t             VFS_working_uid = 0;    // �ļ�ϵͳ������ uid������ʵ�ֶ��û�����

public:
    VFS(): superblock(Superblock()), block_bitmap(Block_bitmap()), inode_bitmap(Inode_bitmap()), inode_table(std::vector<Inode>())
    {

        VFS_login_uid = 0;      // Ĭ�ϵ��� 0 ���û�
        VFS_working_uid = 0;    // Ĭ��cd �� 0 ��Ŀ¼

    }


    // ��ʽ��
    int format()
    {
        {
            std::ofstream File;
            File.open(path);
            File.close();
        }
        // superblock ��ʼ��
        superblock.initialize();
        superblock.write_to_disk(path);
        superblock.read_to_VFS(path);
        superblock.print();
        std::cout << std::endl;

        // block_bitmap ��ʼ��
        block_bitmap.write_to_disk(path);
        block_bitmap.print();
        std::cout << std::endl;

        // inode_bitmap ��ʼ��
        inode_bitmap.write_to_disk(path);
        inode_bitmap.print();
        std::cout << std::endl;

        inode_table = std::vector<Inode>();
        // inode table ��ʼ��
        for(int i = 0; i < 1024*8; ++i)
        {
            Inode temp_inode(i);
            temp_inode.write_to_disk(path);
            inode_table.push_back(temp_inode);
        }

        // ����һ���ļ�ռ�� Block 0����������ļ�ʹ�� block 0 ������ļ�����
        // ����ֱ�� create("", 0), ʹ���ļ����ɲ����޸�ɾ��
        create("odd", 0);

        return 0;   
    }



    int load_from_file()
    {
        inode_table = std::vector<Inode>();
        
        // superblock ��ʼ��
        superblock.read_to_VFS(path);
        superblock.print();
        std::cout << std::endl;

        // block_bitmap ��ʼ��
        block_bitmap.read_to_VFS(path);
        block_bitmap.print();
        std::cout << std::endl;

        // inode_bitmap ��ʼ��
        inode_bitmap.read_to_VFS(path);
        inode_bitmap.print();
        std::cout << std::endl;

        // inode table ��ʼ��
        for(int i = 0; i < 1024*8; ++i)
        {
            Inode temp_inode(i);
            temp_inode.read_to_VFS(path);
            if(!temp_inode.is_null())
            {
                temp_inode.load_blocks(path);  
            }          
            inode_table.push_back(temp_inode);
        }
        return 0;        
    }




    // Ѱ���ļ�����
    // �� working uid Ŀ¼��Ѱ�Ҷ�Ӧ�ļ������� FIND_FALSE ��ʾѰ��ʧ��
    uint32_t find(std::string str_name)
    {
        uint32_t inode_id = FIND_FALSE;
        for(uint32_t i = 0; i < inode_table.size(); ++i)
        {
            if(inode_table[i].get_i_name() == str_name && inode_table[i].get_uid() == VFS_working_uid)
            {
                inode_id = i;
            }
        }
        if(inode_id == FIND_FALSE)
        {
            std::cout << "In uid " << (uint32_t)VFS_working_uid << ", no such file or directory!" << std::endl;
            return FIND_FALSE;
        }
        return inode_id;
    }




    // �����ļ�����
    int create(std::string str_name, uint32_t size_of_block)
    {

        std::cout << "create file: " + str_name << std::endl;

        if(superblock.get_free_inode_count() == 0)
        {
            std::cout << "No free inode!" << std::endl;
            return 0;
        }
        if(size_of_block > superblock.get_free_blocks_count())
        {
            std::cout << "No enough blocks!" << std::endl;
            return 0;
        }
        if(size_of_block > 1024 * 4)
        {
            std::cout << "File size limitedbreak!" << std::endl;
            return 0;
        }
        for(uint32_t i = 0; i < inode_table.size(); ++i)
        {
            if(inode_table[i].get_i_name() == str_name && inode_table[i].get_uid() == VFS_working_uid)
            {
                std::cout << "In uid " << VFS_working_uid <<  " File has been exist!" << std::endl;
                return 0;
            }
        }

        // ѡȡ inode
        uint32_t inode_id = inode_bitmap.get_next_free_inode();
        // ��ʼ��ѡȡ�� inode
        inode_table[inode_id] = Inode(inode_id, str_name, VFS_working_uid);
        inode_bitmap.set_inode(inode_id);
        std::cout << "use inode: " << inode_id << "; ";
        
        // ѡȡһ�������� block, �����ļ��и� block reset
        std::cout << "use blocks: ";
        uint32_t indirect_block = block_bitmap.get_next_free_inode();
        inode_table[inode_id].set_indirect_block(indirect_block, path);

        // block_bitmap ���
        block_bitmap.set_block(indirect_block);
        std::cout << indirect_block << " ";

        // ѡȡ������ݵ� block����ÿ�ζ����ڴ������ø� block������ inode һ��д��
        for(uint32_t i = 0; i < size_of_block; ++i)
        {
            uint32_t temp_id = block_bitmap.get_next_free_inode();
            // ��� block
            inode_table[inode_id].add_block(temp_id);
            // block_bitmap ���
            block_bitmap.set_block(temp_id);
            std::cout << temp_id << " ";
        }
        std::cout << std::endl;

        // superblock ��Ӧ��Ǹ���
        superblock.set_free_blocks_count(superblock.get_free_blocks_count() - 1 - size_of_block);
        superblock.set_free_inode_count(superblock.get_free_inode_count() - 1);

        // ���漰�Ķ�д���ļ�
        inode_bitmap.write_to_disk(path);
        block_bitmap.write_to_disk(path);
        inode_table[inode_id].write_to_disk(path);
        superblock.write_to_disk(path);
        return 0;
    }




    // ɾ���ļ�����
    int remove(std::string str_name)
    {

        std::cout << "remove file: " + str_name << std::endl;

        uint32_t inode_id = find(str_name);
        if(inode_id == FIND_FALSE)
        {
            return 0;
        }

        // ��ȡ��Ҫ�ͷŵ� block �б�
        std::vector<uint32_t> free_blocks = inode_table[inode_id].get_delete_blocks_ids();
        // �ͷ� block
        for(uint32_t i = 0; i < free_blocks.size(); ++i)
        {
            // �޸� block_bitmap ���
            block_bitmap.reset_block(free_blocks[i]);
        }
        // �ͷ� inode
        inode_bitmap.reset_inode(inode_id);
        // �޸� inode bitmap ���
        inode_table[inode_id] = Inode(inode_id);

        // superblock ��Ӧ��¼�޸�
        superblock.set_free_blocks_count(superblock.get_free_blocks_count() + free_blocks.size());
        superblock.set_free_inode_count(superblock.get_free_inode_count() + 1);


        block_bitmap.write_to_disk(path);
        inode_bitmap.write_to_disk(path);
        inode_table[inode_id].write_to_disk(path);
        superblock.write_to_disk(path);

        std::cout << "release block count: " << free_blocks.size() << std::endl;
        std::cout << "release inode id: " << inode_id << std::endl;
        return 0;
    }





    // �ض��ļ�����
    int truncate(std::string str_name)
    {

        std::cout << "truncate file: " + str_name << std::endl;

        uint32_t inode_id = find(str_name);
        if(inode_id == FIND_FALSE)
        {
            return 0;
        }

        // ��ȡ��Ҫ�ͷŵ� block �б�
        std::vector<uint32_t> free_blocks = inode_table[inode_id].get_truncate_block_ids();
        // �ͷ� block
        for(uint32_t i = 0; i < free_blocks.size(); ++i)
        {
            block_bitmap.reset_block(free_blocks[i]);
        }

        inode_table[inode_id].truncate();

        superblock.set_free_blocks_count(superblock.get_free_blocks_count() + free_blocks.size());

        block_bitmap.write_to_disk(path);
        inode_table[inode_id].write_to_disk(path);
        superblock.write_to_disk(path);

        std::cout << "release block count: " << free_blocks.size() << std::endl;
        return 0;
    }





    // �����ļ�����
    int increase(std::string str_name, uint32_t inc_count)
    {

        std::cout << "increase file: " + str_name << std::endl;

        if(inc_count > superblock.get_free_blocks_count())
        {
            std::cout << "NOT ENOUGH BLOCKS!" << std::endl; 
            return 0;           
        }
        uint32_t inode_id = find(str_name);
        if(inode_id == FIND_FALSE)
        {
            return 0;
        }
        if(inode_table[inode_id].get_used_blocks_count() + inc_count > 256)   
        {
            std::cout << "FILE SIZE LIMITEDBREAK!" << std::endl;
            return 0;
        }

        // ѡȡ������ݵ� block
        std::cout << "use blocks: ";
        for(uint32_t i = 0; i < inc_count; ++i)
        {
            uint32_t temp_id = block_bitmap.get_next_free_inode();
            inode_table[inode_id].add_block(temp_id);
            block_bitmap.set_block(temp_id);
            std::cout << temp_id << " ";
        }
        std::cout << std::endl;

        block_bitmap.write_to_disk(path);
        inode_table[inode_id].write_to_disk(path);

        superblock.set_free_blocks_count(superblock.get_free_blocks_count() - inc_count);

        return 0;
    }





    // �ļ�����������
    int rename(std::string name_str, std::string rename_str)
    {
        uint32_t inode_id = find(name_str);
        if(inode_id == FIND_FALSE)
        {
            return 0;
        }
        inode_table[inode_id].set_i_name(rename_str);
        return 0;
    }





    // д�ļ�������ʹ�����λ��ָ��
    int write(std::string str_name, char* input, uint32_t file_offset ,uint32_t size)
    {
        uint32_t inode_id = find(str_name);
        if(inode_id == FIND_FALSE)
        {
            return 0;
        }
        if(file_offset + size > inode_table[inode_id].get_truncate_block_ids().size() * 1024)
        {
            std::cout << "OFFSET OUT OF INDEX!" << std::endl;
            return 0;
        }
        inode_table[inode_id].write(input, file_offset, size);
        inode_table[inode_id].write_to_disk(path);
        return 0;
    }





    // ���ļ����������ָ��
    int read(std::string str_name, char* ouput, uint32_t file_offset ,uint32_t size)
    {
        uint32_t inode_id = find(str_name);
        if(inode_id == FIND_FALSE)
        {
            return 0;
        }
        if(file_offset + size > inode_table[inode_id].get_truncate_block_ids().size() * 1024)
        {
            std::cout << "OFFSET OUT OF INDEX!" << std::endl;
            return 0;
        }
        inode_table[inode_id].read(ouput, file_offset, size);
        inode_table[inode_id].write_to_disk(path);
        return 0;
    }





    // ��ʾָ���ļ�����Ϣ����
    int ls(std::string str_name)
    {
        uint32_t inode_id = find(str_name);
        if(inode_id == FIND_FALSE)
        {
            return 0;
        }
        inode_table[inode_id].ls_print();
        return 0;
    }





    // ��ʾ��ǰ uid �������ļ�����
    int ls_i()
    {
        std::cout << "free inodes count: " << superblock.get_free_inode_count() << std::endl;
        std::cout << "free blocks count: " << superblock.get_free_blocks_count() << std::endl;
        for(uint32_t i = 0; i < inode_table.size(); ++i)
        {
            if(!inode_table[i].is_null() && inode_table[i].get_uid() == VFS_working_uid)
            {
                inode_table[i].ls_i_print();
            }
        }
        return 0;
    }





    // �ȴ�����������н���ʵ��
    int wait_op()
    {
        std::string op_str = "";
        std::string name_str = "";
        uint32_t    size = 0;

        while(op_str != "exit")
        {
            std::cin >> op_str;
            if(op_str == "create")
            {
                std::cin >> name_str >> size;
                create(name_str, size);
            }
            else if (op_str == "remove")
            {
                std::cin >> name_str;
                remove(name_str);
            }
            else if (op_str == "truncate")
            {
                std::cin >> name_str;
                truncate(name_str);
            }
            else if (op_str == "increase")
            {
                std::cin >> name_str >> size;
                increase(name_str, size);
            }
            else if (op_str == "ls_i" || op_str == "ls-i")
            {
                ls_i();
            }
            else if (op_str == "write")
            {
                std::string temp_str;
                std::cin >> name_str >> temp_str;
                if(temp_str.size() > 200)
                {
                    std::cout << "size > 100 cannot write from command" << std::endl;
                    continue;
                }
                char input[200] = {};
                temp_str.copy(input, temp_str.size());
                write(name_str, input, 0, temp_str.size());
            }
            else if (op_str == "read")
            {
                std::cin >> name_str >> size;
                if(size > 200)
                {
                    std::cout << "size > 100 cannot read from command" << std::endl;
                    continue;
                }
                char output[200] = {};
                read(name_str, output, 0, size);
                std::cout << output << std::endl;
            }
            else if (op_str == "ls")
            {
                std::cin >> name_str;
                ls(name_str);
            }
            else if (op_str == "find")
            {
                std::cin >> name_str;
                uint32_t inode_id = find(name_str);
                if(inode_id == FIND_FALSE)
                {
                    return 0;
                }
                inode_table[inode_id].ls_print();
            }
            else if (op_str == "cd")
            {
                std::cin >> VFS_working_uid;
            }
            else if (op_str == "login")
            {
                std::cin >> VFS_login_uid;
            }
            else if (op_str == "rename")
            {
                std::string rename_str;
                std::cin >> name_str >> rename_str;
                rename(name_str, rename_str);
            }
            else if (op_str == "id")
            {
                std::cout << "current working id: " << (int)VFS_working_uid;
            }
            else if (op_str == "exit")
            {
                return 0;
            }  
        }
        return 0;
    }





    // ��������
    int test()
    {
        std::cout << "--- ���� test1.txt ---" << std::endl;
        create("test1.txt", 2);
        ls_i();
        std::cout << std::endl;

        std::cout << "--- ���� test2.txt ---" << std::endl;
        create("test2.txt", 9);
        ls_i();
        std::cout << std::endl;

        std::cout << "--- ɾ�� test1.txt ---" << std::endl;
        remove("test1.txt");
        ls_i();
        std::cout << std::endl;

        std::cout << "--- �ض� test2.txt ---" << std::endl;
        truncate("test2.txt");
        ls("test2.txt");
        std::cout << std::endl;

        std::cout << "--- ���� test2.txt ---" << std::endl;
        increase("test2.txt", 1);
        ls("test2.txt");
        std::cout << std::endl;

        std::cout << "--- ���� test2.txt ---" << std::endl;
        create("test2.txt", 5);
        ls_i();
        std::cout << std::endl;

        std::cout << "--- �л��� uid 1 ---" << std::endl;
        VFS_working_uid = 1;
        ls_i();
        std::cout << std::endl;

        std::cout << "--- ���� test2.txt ---" << std::endl;
        create("test2.txt", 5);
        ls_i();
        std::cout << std::endl;


        char input[20] = "input_test";
        char output[20] = "output_test";

        // ��һ������ input д��block3�� �ļ��� offset 0x00101800 
        // �ڶ��β���д�� block 3 ĩβ���ļ��� offset 0x00101C00 - 2
        // ����д�� block 5���ļ��� offset 0x00102000
        // �м�� block 4 �� inode 1 �� һ���������ݣ��ļ��� offset 0x00101C00
        write("test2.txt", input, 0, 20);
        write("test2.txt", input, 1024 - 2, 20);
        read("test2.txt", output, 1024 - 2, 20);
        std::cout << "--- �� test2.txt offset 1024-2 ��ȡ�� ---" << std::endl;
        std::cout << output << std::endl << std::endl;

        return 0;
    }
};

#endif