#include <curl/curl.h>
#include <iostream>
#include <string>
#include <fstream>

static size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
    /*
        buffer - the data coming in
        size - size of each element in bytes
        nmemb - number of elements
        userp - user pointer (in this case the file)
    */

    // calculate the real size of the data
    size_t realSize = size * nmemb;

    // get the file from the userp
    std::ofstream *file = static_cast<std::ofstream *>(userp);

    // write the buffer contents into the file.
    file->write(static_cast<const char *>(buffer), realSize);

    return realSize;
}

void fetchData()
{
    // file from which to download data
    std::string url = "https://publicdata.caida.org/datasets/as-relationships/serial-2/20151201.as-rel2.txt.bz2";

    CURL *curl;
    CURLcode result = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (result)
    {
        std::cerr << "Error: " << curl_easy_strerror(result) << std::endl;
        return;
    }

    curl = curl_easy_init();
    if (curl == nullptr)
    {
        std::cerr << "Http Request Failed" << std::endl;
        return;
    }
    std::ofstream out("caida_data.bz2", std::ios::binary);

    curl_easy_setopt(curl, CURLOPT_URL, "https://publicdata.caida.org/datasets/as-relationships/serial-2/20151201.as-rel2.txt.bz2");
    // function used to write data
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    // function to write data to file
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);

    result = curl_easy_perform(curl);
    if (result != CURLE_OK)
    {
        std::cerr << "Error: " << curl_easy_strerror(result) << std::endl;
        return;
    }

    // cleanup required streams.
    out.close();
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    std::cout << "successfully downloaded data!" << std::endl;
}
