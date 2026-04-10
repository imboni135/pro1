<?php
$conn = new mysqli("localhost", "root", "", "livestock_db");

// REGISTER ANIMAL
if (isset($_POST['save_animal'])) {
    $tagId = $_POST['tagId'];
    $name = $_POST['name'];
    $animalType = $_POST['animalType'];
    $sex = $_POST['sex'];
    $breed = $_POST['breed'];
    $birthdate = $_POST['birthdate'];
    $isPregnant = $_POST['isPregnant'];
    $isSick = $_POST['isSick'];
    $ownerContact = $_POST['ownerContact'];

    $conn->query("INSERT INTO animal VALUES(
        '$tagId','$name','$animalType','$sex','$breed',
        '$birthdate','$isPregnant','$isSick','$ownerContact',NOW()
    )");
}

// DELETE ANIMAL
if (isset($_GET['delete'])) {
    $tagId = $_GET['delete'];
    $conn->query("DELETE FROM animal WHERE tagId='$tagId'");
    header("Location: caw1.php");
    exit();
}

// DELETE HEALTH RECORD
if (isset($_GET['delete_health'])) {
    $id = $_GET['delete_health'];
    $conn->query("DELETE FROM health_records WHERE id='$id'");
    header("Location: caw1.php");
    exit();
}

// SAVE HEALTH RECORD
if (isset($_POST['save_health'])) {
    $conn->query("INSERT INTO health_records(tagId,type,startDate,endDate,nextEventDate,vetName,vetContact,notes)
    VALUES(
        '{$_POST['tagId']}',
        '{$_POST['type']}',
        '{$_POST['startDate']}',
        '{$_POST['endDate']}',
        '{$_POST['nextEventDate']}',
        '{$_POST['vetName']}',
        '{$_POST['vetContact']}',
        '{$_POST['notes']}'
    )");
}
?>
    <?php
$conn = new mysqli("localhost", "root", "", "livestock_db");

// ================= API FOR ESP32 =================
if (isset($_GET['tagId']) && !isset($_POST['save_animal'])) {

    $tagId = $_GET['tagId'];

    $res = $conn->query("SELECT * FROM animal WHERE tagId='$tagId'");

    header('Content-Type: application/json');

    if ($res->num_rows > 0) {

        $animal = $res->fetch_assoc();

        $health = $conn->query("
            SELECT * FROM health_records 
            WHERE tagId='$tagId' 
            ORDER BY startDate DESC 
            LIMIT 1
        ");

        $latest = $health->fetch_assoc();

        echo json_encode([
            "status" => "found",
            "name" => $animal['name'],
            "isPregnant" => (bool)$animal['isPregnant'],
            "isSick" => (bool)$animal['isSick'],
            "ownerContact" => $animal['ownerContact'],
            "health" => $latest
        ]);

    } else {
        echo json_encode(["status" => "not_found"]);
    }

    exit(); // 🔴 VERY IMPORTANT → stop HTML from loading
}
?>



<!DOCTYPE html>
<html>
<head>
<title>Livestock Dashboard</title>
<style>
body {display:flex;font-family:Arial;margin:0;}
.sidebar {width:220px;background:#2c3e50;color:white;height:100vh;padding:20px;}
.sidebar li {padding:10px;cursor:pointer;}
.sidebar li:hover {background:#34495e;}
.main {flex:1;padding:20px;background:#ecf0f1;}
input,select,textarea {width:100%;margin:5px 0;padding:8px;}
button {background:green;color:white;padding:10px;border:none;}
table {width:100%;background:white;border-collapse:collapse;}
td,th {padding:8px;border:1px solid #ccc;}
a {text-decoration:none;}
.delete {color:red;font-weight:bold;}
</style>

<script>
function showSection(id){
    document.querySelectorAll('.section').forEach(s=>s.style.display='none');
    document.getElementById(id).style.display='block';
}
</script>
</head>

<body>

<div class="sidebar">
<h2>🐄 Livestock</h2>
<ul>
<li onclick="showSection('register')">Register</li>
<li onclick="showSection('view')">View Animals</li>
<li onclick="showSection('search')">Search Animal</li>
<li onclick="showSection('health')">Health</li>
</ul>
</div>

<div class="main">

<!-- REGISTER -->
<div id="register" class="section">
<h2>Register Animal</h2>
<form method="POST">
<input name="tagId" placeholder="Tag ID" required>
<input name="name" placeholder="Name" required>

<select name="animalType">
<option>Cattle</option><option>Goat</option>
<option>Sheep</option><option>Swine</option>
</select>

<select name="sex">
<option>Male</option><option>Female</option>
</select>

<input name="breed" placeholder="Breed">
<input type="date" name="birthdate">

<select name="isPregnant">
<option value="0">Not Pregnant</option>
<option value="1">Pregnant</option>
</select>

<select name="isSick">
<option value="0">Healthy</option>
<option value="1">Sick</option>
</select>

<input name="ownerContact" placeholder="Contact">

<button name="save_animal">Save</button>
</form>
</div>

<!-- VIEW -->
<div id="view" class="section" style="display:none;">
<h2>All Animals</h2>
<table>
<tr><th>Tag</th><th>Name</th><th>Type</th><th>Action</th></tr>

<?php
$res = $conn->query("SELECT * FROM animal");
while($row = $res->fetch_assoc()){
?>
<tr>
<td><?php echo $row['tagId']; ?></td>
<td><?php echo $row['name']; ?></td>
<td><?php echo $row['animalType']; ?></td>
<td>
<a href="?delete=<?php echo $row['tagId']; ?>" 
class="delete"
onclick="return confirm('Delete this animal?')">
Delete
</a>
</td>
</tr>
<?php } ?>

</table>
</div>

<!-- SEARCH -->
<div id="search" class="section" style="display:none;">
<h2>Search Animal</h2>

<form method="GET">
<input name="tag" placeholder="Enter Tag ID">
<button>Search</button>
</form>

<?php
if (isset($_GET['tag'])) {
    $tag = $_GET['tag'];
    $res = $conn->query("SELECT * FROM animal WHERE tagId='$tag'");
    if($res->num_rows>0){
        $row=$res->fetch_assoc();
        echo "<p><b>Name:</b> {$row['name']}</p>";
        echo "<p><b>Type:</b> {$row['animalType']}</p>";
        echo "<p><b>Breed:</b> {$row['breed']}</p>";
    } else {
        echo "Animal not found";
    }
}
?>
</div>

<!-- HEALTH -->
<div id="health" class="section" style="display:none;">
<h2>Health Records</h2>

<form method="POST">
<input name="tagId" placeholder="Tag ID" required>

<select name="type">
<option value="vaccination">Vaccination</option>
<option value="pregnancy">Pregnancy</option>
<option value="disease">Disease</option>
</select>

<input type="date" name="startDate">
<input type="date" name="endDate">
<input type="date" name="nextEventDate">

<input name="vetName" placeholder="Vet">
<input name="vetContact" placeholder="Contact">
<textarea name="notes"></textarea>

<button name="save_health">Save</button>
</form>

<hr>

<table>
<tr>
<th>Tag</th>
<th>Type</th>
<th>Date</th>
<th>Vet</th>
<th>Action</th>
</tr>

<?php
$res = $conn->query("SELECT * FROM health_records");
while($row = $res->fetch_assoc()){
?>
<tr>
<td><?php echo $row['tagId']; ?></td>
<td><?php echo $row['type']; ?></td>
<td><?php echo $row['startDate']; ?></td>
<td><?php echo $row['vetName']; ?></td>
<td>
<a href="?delete_health=<?php echo $row['id']; ?>" 
class="delete"
onclick="return confirm('Delete this record?')">
Delete
</a>
</td>
</tr>
<?php } ?>

</table>
</div>

</div>
</body>
</html>