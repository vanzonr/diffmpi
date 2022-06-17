// 
//  File:          inifile.cc (or inifile.cxx)
//  Created:       Feb. 2003
//  Last modified: Aug. 24 2006
//  Author:        Ramses van Zon 
//
//  Documentation in doc/Inifiledoc.tex
//

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>
#include "inifile.h"

#define APPEND(fld) \
 int len = strlen(fld);                                  \
 char *appended##fld = new char[sectionnamelen+len+1];   \
 strncpy(appended##fld, sectionname, sectionnamelen+1);  \
 strncpy(appended##fld+sectionnamelen, fld, len+1)

#define ENDAPPEND(fld) delete[] appended##fld

Data::Data(char const * name)
{
   strcpy(keyword, name);
   value[0] = '\0';
   used = false;
}

Inifile::Inifile()
{}

Inifile::~Inifile()
{
   clear();
}

void Inifile::clear()
{
   data.clear();
}

 // void Inifile::write(char const *filename) const
 // {
 //    using namespace std;

 //    vector<Data>::const_iterator k;
 //    ofstream f(filename);

 //    for (k = data.begin(); k != data.end(); k++) 
 //       f << k->keyword << " = " << k->value << '\n';

 //    f.close();
 // }

void Inifile::write(std::ostream &f, const char ch) const
{
   for (std::vector<Data>::const_iterator k = data.begin(); k != data.end(); k++) 
       if (k->used)
           f << ch << k->keyword << " = " << k->value << '\n';
}

void Inifile::write(char const *filename, const char ch) const
{
   using namespace std;
   ofstream f(filename);
   write(f, ch);
   f.close();
}


void Inifile::writecontaining(char const *filename, char const *sectionname) const
{
   using namespace std;

   vector<Data>::const_iterator k;
   ofstream f(filename);
  
   for (k = data.begin(); k != data.end(); k++) 
      if ( strstr( k->keyword, sectionname) )
         f << k->keyword << " = " << k->value << '\n';

   f.close();
}

void Inifile::assign(char const *f, char const * v)
{
   std::vector<Data>::iterator k;
   bool found = false;

   for (k = data.begin(); k != data.end(); k++) 
      if (strcmp(f, k->keyword)==0) 
      {
         strcpy(k->value, v);
         found = true;
      }

   if (!found) 
   {
      data.push_back(Data(f));
      assign(f,v);
   }
}

void Inifile::read(char const *filename)
{
   using namespace std;

   ifstream file(filename);
   char readstr[256];
   char *field;
   char *value;
   int  linecounter=0;
   int  sectionnamelen = 0;
   char *sectionname = 0;
   char buffer[130];

   if (!file) 
   {
      char answer;

      cerr << "File " << filename << " not found.\n"
           << "Continue, with unpredictable results(y/n)? "
           << endl;	
      scanf("%c", &answer);

      if (answer!='y')
         exit(1);
      else
         return;
   } 
   else 
   {
      while (!file.eof()) 
      {
         file.getline(readstr,256);
         linecounter++;
         field = strtok(readstr," \t=\0\n");

         if (field) 
         {
            if (*field!='#') 
            {
               value = strtok(NULL," \t=\0\n");

               if (sectionname)  // in a particular section?
               { 
                  APPEND(field);

                  if (value)
                     assign(appendedfield,value);
                  else
                     assign(appendedfield,"0");

                  ENDAPPEND(field);
               } 
               else 
               {
                  if (value)
                     assign(field,value);
                  else
                     assign(field,"0");
               }
            } 
            else 
            {
               // check for special commands: (case insensitive)

               if (strcasecmp(field,"#INCLUDE")==0) 
               {
                  value = strtok(NULL," \t\0\n");

                  if (value)
                     read(value);
               }

               // New special command "#BEGIN" or "#CASE" is to prepend 
               // the name of the section plus a period to any 
               // variable name.
               // Note : no nesting is allowed at present.
               //        (too confusing)

               if (strcasecmp(field,"#BEGIN")==0 || strcasecmp(field,"#CASE")==0) 
               {
                  value = strtok(NULL," \t\0\n");
               
                  if (value) 
                  {
                     if (*value == '\0')   // empty string?
                     { 
                        sectionname = 0;         // then revert to no class
                        sectionnamelen = 0;
                     } 
                     else 
                     {
                        strncpy(buffer,value,128);
                        strncpy(buffer+strlen(value),".\0",2);
                        sectionname = buffer;
                        sectionnamelen = strlen(sectionname);
                     }
                  }
               }

               if (strcasecmp(field,"#END")==0) 
               {
                  sectionname = 0;
                  sectionnamelen = 0;
               }
            }
         }
      }
   }           

   file.close();
}

Inifile::Inifile(char const *filename)
{
   read(filename);
}

std::vector<Data>::const_iterator Inifile::get(char const *name, bool request) const
{
   std::vector<Data>::const_iterator k;

   for (k = data.begin(); k != data.end(); k++) 
       if (strcmp(name, k->keyword)==0) {
           if (request) 
               k->used = true;
           break;
       }

   return k;
}

bool Inifile::verify(char const * name) const
{
   return get(name)!=data.end();
}

bool Inifile::requested(char const * name) const
{
    std::vector<Data>::const_iterator k = get(name, false);
    if (k==data.end())
        return false;
    else
        return k->used;
}

char * Inifile::get_string_wb(char const *name, char *value, const char *defaultValue) const
{
   bool found = false;
   std::vector<Data>::const_iterator k;
  
   for (k = data.begin(); k != data.end(); k++) 
      if (strcmp(name, k->keyword)==0) 
      {
         strcpy(value, k->value);
         found = true;
      }

   if (!found) 
      strcpy(value, defaultValue);

   return value;
}

char value = '\0';
const char * Inifile::get_string(char const *name, const char *defaultValue) const
{
   std::vector<Data>::const_iterator k=get(name);

   if (k!=data.end()) 
   {
      char *result;
      result = new char[strlen(k->value)+1];
      strcpy(result, k->value);
      return result;
      //return k->value; 
   } 
   else 
   {
      if (defaultValue) 
      {
         char *result;

         result = new char[strlen(defaultValue)+1];
         strcpy(result, defaultValue);

         return result;
      } 
      else 
      {
         char *result;

         result = new char[1];
         strcpy(result, &value);

         return result;
      }
   }
}
 
int Inifile::get_int(char const *name, int defaultValue) const
{
   std::vector<Data>::const_iterator k = get(name);

   if (k != data.end()) 
      return atoi(k->value); 
   else 
      return defaultValue;
}

long Inifile::get_long(char const *name, long defaultValue) const
{
   std::vector<Data>::const_iterator k = get(name);

   if (k != data.end()) 
      return atol(k->value); 
   else 
      return defaultValue;
}

long long Inifile::get_longlong(char const *name, long long defaultValue) const
{
   std::vector<Data>::const_iterator k = get(name);

   if (k!=data.end()) 
#ifdef __GNUC__
#ifndef __DJGPP__
      return atoll(k->value); 
#else
      return (long long)atol(k->value);
#endif
#else
      return (long long)atol(k->value);
#endif
   else 
      return defaultValue;
}

float Inifile::get_float(char const *name, float defaultValue) const
{
   std::vector<Data>::const_iterator k = get(name);

   if (k!=data.end()) 
      return atof(k->value); 
   else 
      return defaultValue;
}

double Inifile::get_double(char const *name, double defaultValue) const
{
   std::vector<Data>::const_iterator k = get(name);

   if (k!=data.end()) 
      return atof(k->value); 
   else 
      return defaultValue;
}

#ifdef QUADRUPLEP
long double Inifile::get_longdouble(char const *name, long double defaultValue) const
{
   std::vector<Data>::const_iterator k = get(name);

   if (k!=data.end()) 
      return strtold(k->value,(char**)0); 
   else 
      return defaultValue;
}
#endif

//
 
void Inifile::set_int(char const *name, int i)
{
   char tmp[128];

   sprintf(tmp, "%d", i);
   assign(name, tmp);
}

void Inifile::set_long(const char *name, long l)
{
   char tmp[128];

   sprintf(tmp, "%ld", l);
   assign(name, tmp);
}

void Inifile::set_longlong(const char *name, long long ll)
{
   char tmp[128];

   sprintf(tmp, "%lld", ll);
   assign(name, tmp);
}

void Inifile::set_float(const char *name, float f)
{
   char tmp[128];

   sprintf(tmp, "%f", f);
   assign(name, tmp);
}

void Inifile::set_double(const char *name, double d)
{
   char tmp[128];

   sprintf(tmp, "%lf", d);
   assign(name, tmp);
}

#ifdef QUADRUPLEP
void Inifile::set_longdouble(const char *name, long double d)
{
   char tmp[128];

   sprintf(tmp, "%Lf", d);
   assign(name, tmp);
}
#endif

//

int Inifile::getorset_int(char const *name, int i) 
{
   std::vector<Data>::iterator k;

   for (k = data.begin(); k != data.end(); k++) 
      if (strcmp(name, k->keyword)==0) 
         break;

   if (k!=data.end()) 
      return atoi(k->value); 
   else 
   {
      char v[128];

      sprintf(v, "%d", i);
      strcpy(k->value, v);

      return i;
   }
}

long Inifile::getorset_long(char const *name, long l) 
{
   std::vector<Data>::iterator k;

   for (k = data.begin(); k != data.end(); k++) 
      if (strcmp(name, k->keyword)==0) 
         break;

   if (k!=data.end()) 
      return atol(k->value); 
   else 
   {
      char v[128];
   
      sprintf(v, "%ld", l);
      strcpy(k->value, v);

      return l;
   }
}

long long Inifile::getorset_longlong(char const *name, long long ll) 
{
   std::vector<Data>::iterator k;

   for (k = data.begin(); k != data.end(); k++) 
      if (strcmp(name, k->keyword)==0) 
         break;

   if (k!=data.end()) 
#ifdef __GNUC__
#ifndef __DJGPP__
      return atoll(k->value);
#else
      return (long long)atol(k->value);
#endif
#else
      return (long long)atol(k->value);
#endif
   else 
   {
      char v[128];

      sprintf(v, "%lld", ll);
      strcpy(k->value, v);

      return ll;
   }
}

float Inifile::getorset_float(char const *name, float f) 
{
   std::vector<Data>::iterator k;

   for (k = data.begin(); k != data.end(); k++) 
      if (strcmp(name, k->keyword)==0) 
         break;

   if (k!=data.end()) 
      return atof(k->value); 
   else 
   {
      char v[128];

      sprintf(v, "%f", f);
      strcpy(k->value, v);

      return f;
   }
}

double Inifile::getorset_double(char const *name, double d) 
{
   std::vector<Data>::iterator k;

   for (k = data.begin(); k != data.end(); k++) 
      if (strcmp(name, k->keyword)==0) 
         break;

   if (k!=data.end()) 
      return atof(k->value); 
   else 
   {
      char v[128];

      sprintf(v, "%lf", d);
      strcpy(k->value, v);

      return d;
   }
}

#ifdef QUADRUPLEP
long double Inifile::getorset_longdouble(char const *name, long double ld) 
{
   std::vector<Data>::iterator k;

   for (k = data.begin(); k != data.end(); k++) 
      if (strcmp(name, k->keyword)==0) 
         break;

   if (k!=data.end()) 
      return strtold(k->value,(char**)0); 
   else 
   {
      char v[128];
   
      sprintf(v, "%Lf", ld);
      strcpy(k->value, v);

      return ld;
   }
}
#endif


InifileSection::InifileSection() 
   : ini(0), sectionname(0), sectionnamelen(0)
{}

#define MAXSTRLEN 128

InifileSection::InifileSection(const InifileBase &inib, const char *secname)
{
   ini = &inib;
   if (secname) 
   {
      sectionnamelen = strlen(secname)+1;    // includes the separating period
      sectionname = new char[sectionnamelen+1];
      strncpy(sectionname, secname, sectionnamelen-1); // no period yet
      *(sectionname+sectionnamelen-1) = '.';           // add the period
      *(sectionname+sectionnamelen) = '\0';  // zero terminate for good measure
   } 
}

InifileSection::~InifileSection()
{
   delete[] sectionname;
}
 
bool InifileSection::verify(char const * name) const
{
   bool b=true;

   APPEND(name);

   if (!ini->verify(appendedname)) 
      b = ini->verify(name);

   ENDAPPEND(name);

   return b;
}

bool InifileSection::requested(char const * name) const
{
   bool b=true;

   APPEND(name);

   if (!ini->requested(appendedname)) 
      b = ini->requested(name);

   ENDAPPEND(name);

   return b;
}

char* InifileSection::get_string_wb(char const *name, char *value, const char *defaultValue) const
{
   char *strng;

   APPEND(name);
   strng = ini->get_string_wb( ini->verify(appendedname) ? appendedname : name , value, defaultValue);
   ENDAPPEND(name);

   return strng;
}

const char* InifileSection::get_string(char const *name, const char *defaultValue) const
{
   const char *strng;

   APPEND(name);
   strng = ini->get_string( ini->verify(appendedname) ? appendedname : name, defaultValue);
   ENDAPPEND(name);

   return strng;
}

int InifileSection::get_int(char const *name, int defaultValue) const
{
   int i;

   APPEND(name);
   i = ini->get_int( ini->verify(appendedname) ? appendedname : name , defaultValue);
   ENDAPPEND(name);

   return i;
}

long InifileSection::get_long(char const *name, long defaultValue) const
{
   long l;

   APPEND(name);
   l = ini->get_long( ini->verify(appendedname) ? appendedname : name, defaultValue);
   ENDAPPEND(name);

   return l;
}

long long InifileSection::get_longlong(char const *name, long long defaultValue) const
{
   long long ll;

   APPEND(name);
   ll = ini->get_longlong( ini->verify(appendedname) ? appendedname : name, defaultValue);
   ENDAPPEND(name);

   return ll;
}

float InifileSection::get_float(char const *name, float defaultValue) const
{
   float f;

   APPEND(name);
   f = ini->get_float( ini->verify(appendedname) ? appendedname : name , defaultValue);
   ENDAPPEND(name);

   return f;
}

double InifileSection::get_double(char const *name, double defaultValue) const
{
   double d;

   APPEND(name);
   d = ini->get_double( ini->verify(appendedname) ? appendedname : name , defaultValue);
   ENDAPPEND(name);

   return d;
}

#ifdef QUADRUPLEP
long double InifileSection::get_longdouble (char const *name, long double defaultValue) const
{
   long double ld;

   APPEND(name);
   ld = ini->get_longdouble( ini->verify(appendedname) ? appendedname : name, defaultValue);
   ENDAPPEND(name);

   return ld;
}
#endif


