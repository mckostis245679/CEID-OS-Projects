#!/bin/bash


insert_data() {
    echo "Enter the file path (or leave blank to input manually):"
    read filepath

    if [ -z "$filepath" ]; then
        filename="./passenger_data.csv"
    elif [ -f "$filepath" ]; then
        echo "Enter passenger data in the format:"
        echo "[code],[fullname],[age],[country],[status (Passenger/Crew)],[rescued (Yes/No)]"
        echo "Type 'END' when you are done."

        while true; do
            read line
            if [ -z "$line" ]; then
                break
            fi


            if [[ "$line" =~ ^[A-Za-z0-9]+,[^,]+,[0-9]+,[^,]+,(Passenger|Crew),(Yes|No)$ ]]; then
                echo "$line" >> "$filepath"
            elif [ "$line" = "END" ]; then
		break
	    else
                echo "Invalid format. Please try again."
            fi
        done
    else
        echo "The file does not exist, please check the path."
    fi
}

search_passenger(){
	 echo "Enter the file path (or leave blank to input manually):"
         read filepath

         if [ ! -f "$filepath" ]; then
		echo "$filepath doesnt exist"
		return
	 fi

	echo "Give the persons Name or Surname to search for"
	read person_to_search
	
	match=$(grep -i "$person_to_search" "$filepath")

	if [ -z "$match" ]; then
		echo "No match found for '$person_to_search'."
	else
		echo "Matches:"
		echo "$match"
	fi

}

update_passenger(){
	echo "update"
}


display_file(){
	echo "display"
}


generate_reports(){
	echo "generate"
}



if [ $1 == "insert" ]; then
	insert_data
elif [ $1 == "search" ]; then
	search_passenger
elif [ $1 == "update" ]; then
	update_passenger
elif [ $1 == "display" ]; then
	display_file
elif [ $1 == "generate" ]; then
	generate_reports
else
	echo "Type the following to access functions: insert, search, update, display, reports"
fi
