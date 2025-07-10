import scala.io.Source
import java.net.URL
import sys.process._
import scala.io.Source

import scala.concurrent.{Await, Future}
import scala.concurrent.duration._
import scala.concurrent.ExecutionContext.Implicits.global

def isImage(url: String): Boolean = {
    val connection = URL(url).openConnection()
    connection.setDoOutput(false)
    val contentType = connection.getHeaderField("Content-Type")

    contentType match {
        case null => return false
        case contentType: String => contentType.startsWith("image/")
    }
}

@main
def imagePager(kittyString: String, NewsboatArticle: String) = {
    val kitty = kittyString.toBoolean

    val urlPattern = """https?://\S+""".r

    val images = Source.fromFile(NewsboatArticle)
        .getLines
        .flatMap(urlPattern.findAllIn(_))
        .map(url => Future { (url, isImage(url)) })
        .toList

    val filteredImages = Await.result(Future.sequence(images), Duration.Inf)
        .collect { case (url, true) => url }

    if (!filteredImages.isEmpty) {
        kitty match {
            case true => {
                //val cols = s"tput cols".!!
                //val lines = s"tput lines".!!
                //val dimensions = s"${cols}x${lines}@0x0".replaceAll("\n", "")

                // This code is mostly from @heussd's kitty-imager-pager.sh bash script
                // for rendering images with kitty (pull request #1956 on Newsboat)
                val kittyImages = filteredImages
                    .map(image => s"kitty +kitten icat --hold --scale-up --place \"$$dims\" $image")
                    .toSeq

                (Seq(
                    "dims=\"$(tput cols)x$(tput lines)@0x0\"",
                    "clear",
                    "kitty +kitten icat --clear",
                ) ++ kittyImages :+ "clear").foreach(println)
            }
            case false => s"feh ${filteredImages.mkString(" ")}".!!
        }
    }
}
