#include <iostream>
#include <curl/curl.h>

int main() {
    CURL *curl;
    CURLcode res;
    
    curl_mime *form = NULL;
    curl_mimepart *field = NULL;
    
    curl_global_init(CURL_GLOBAL_ALL);
    
    curl = curl_easy_init();
    if (curl) {
        /* Create the form */
        form = curl_mime_init(curl);
        if (form) {
            /* Fill in the file upload field */
            field = curl_mime_addpart(form);
            curl_mime_name(field, "image");
            curl_mime_filedata(field, "data/20240302_120654_836-image2.jpg");
        
            /* Fill in the filename field */
            field = curl_mime_addpart(form);
            curl_mime_name(field, "timestamp");
            curl_mime_data(field, "12345678", CURL_ZERO_TERMINATED);
        
            /* what URL that receives this POST */
            curl_easy_setopt(curl, CURLOPT_URL, "https://5191dff8-f273-4a03-8fd2-b03e52934e9f.mock.pstmn.io/feed");

            curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
        
            /* Perform the request, res gets the return code */
            res = curl_easy_perform(curl);
            /* Check for errors */
            if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));
        
            /* always cleanup */
            curl_easy_cleanup(curl);
        
            /* then cleanup the form */
            curl_mime_free(form);
        }
    }

    std::cout << "\n";

    // CURL *curl;
    // CURLcode result;
    
    // curl_global_init(CURL_GLOBAL_ALL);

    // curl = curl_easy_init();
    // if (curl) {
    //     struct curl_httppost *formpost = NULL;
    //     struct curl_httppost *lastptr = NULL;
    //     struct curl_slist *headerlist = NULL;
    //     static const char buf[] = "Expect:";

    //     /* Add form field */
    //     // This function is deprecated. Use curl_mime_init instead.
    //     curl_formadd(&formpost,
    //                  &lastptr,
    //                  CURLFORM_COPYNAME, "file",
    //                  CURLFORM_FILE, "data/20240302_120654_836-image2.jpg",
    //                  CURLFORM_END);

    //     /* Set the URL for the POST request */
    //     curl_easy_setopt(curl, CURLOPT_URL, "https://5191dff8-f273-4a03-8fd2-b03e52934e9f.mock.pstmn.io/feed");

    //     /* Set the form in the request */
    //     curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

    //     /* Set custom headers */
    //     headerlist = curl_slist_append(headerlist, buf);
    //     curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);

    //     /* Perform the request, res will get the return code */
    //     result = curl_easy_perform(curl);

    //     /* Always cleanup */
    //     curl_easy_cleanup(curl);

    //     /* Free the form */
    //     curl_formfree(formpost);

    //     /* Free custom headers */
    //     curl_slist_free_all(headerlist);

    //     /* Check for errors */
    //     if(result != CURLE_OK)
    //         fprintf(stderr, "curl_easy_perform() failed: %s\n",
    //                 curl_easy_strerror(result));
    // }

    // curl_global_cleanup();
    // std::cout << "\n";

    return 0;
}