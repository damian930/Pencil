#ifndef OS_OS_CPP
#define OS_OS_CPP

#include "os_core.h"

// todo: Style this file better
// todo: Make a stylistic distinciton in os_core.h between the per os stuff and this

U64 os_file_get_size(OS_File file)
{
  OS_File_properties properties = os_file_get_properties(file);
  U64 size = properties.size;
  return size;
}

// - file manipulations
U64 os_file_write(OS_File file, Data_buffer buffer)
{
  U64 bytes_written = os_file_write_range(file, buffer, 0, buffer.count);
  return bytes_written;
}

void os_file_clear_at_path(Str8 path)
{
  OS_File file = os_file_open(path, OS_File_access__read);
  if (os_file_is_valid(file))
  {
    os_file_close(&file);
    file = os_file_open(path, OS_File_access__write);
  } 
  os_file_close(&file);
}

void os_file_copy(Str8 original_file_path, Str8 new_file_path)
{
  Scratch scratch = get_scratch(0, 0);
  
  OS_File file = os_file_open(original_file_path, OS_File_access__visible_read);
  U64 size = os_file_get_size(file);
  Data_buffer buffer = data_buffer_make(scratch.arena, size);
  U64 bytes_read = os_file_read(file, &buffer);
  HandleLater(bytes_read == buffer.count);
  os_file_close(&file);

  OS_File new_file = os_file_open(new_file_path, OS_File_access__visible_write);
  U64 bytes_written = os_file_write(new_file, buffer);
  HandleLater(bytes_written == buffer.count);
  os_file_close(&new_file);

  end_scratch(&scratch);
}





#endif
