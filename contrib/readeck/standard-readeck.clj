#!/usr/bin/env bb

(ns standard-readeck
  (:require [cheshire.core :as json]
            [babashka.curl :as curl]
            [babashka.tasks :as tasks]
            [clojure.string :as s]))

;; User Configuration

(def my-readeck-address "<your-address-here>")
(def my-token "<your-token-here>")

;; Optionally you can set the default label(s) to be a vector such as
;; ["label-1" "label-2"]
;; Leaving it as an empty vector adds no labels on bookmark creation
(def default-labels [])

;; End User Configuration


(def readeck-endpoint (str my-readeck-address "/api/bookmarks"))

(def auth-headers {"Accept" "application/json"
                   "Authorization" (str "Bearer " my-token)
                   "content-type" "application/json"})

(defn add-bookmark [endpoint headers]
  (let [[url title desc] *command-line-args*
        body {"title" title
              "url" url}
        body-with-labels (if (seq default-labels)
                           (conj body {"labels" default-labels})
                           body)]
    (curl/post endpoint {:headers headers
                         :body (json/generate-string body-with-labels)})))

(add-bookmark readeck-endpoint auth-headers)
